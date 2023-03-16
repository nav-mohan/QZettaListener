#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),m_ui(new Ui::MainWindow)
{
    // m_regexPattern = QRegularExpression("\\<\?xml.*?<\/ZettaClipboard>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption);
    m_regexPattern = QRegularExpression(
        // "<ZettaClipboard.*?<\/ZettaClipboard>",
        "<LogEvent.*?<\/LogEvent>",
        QRegularExpression::MultilineOption|
        QRegularExpression::DotMatchesEverythingOption
    );
    m_ui->setupUi(this);
    m_tcpServer = new QTcpServer();
    m_xmlReader = new QXmlStreamReader();
    if(m_tcpServer->listen(QHostAddress::Any, 10000))
        qDebug("LISTENING...");

    connect(m_tcpServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    m_ui->statusBar->showMessage("Server is listening...");
}

MainWindow::~MainWindow()
{
    foreach(QTcpSocket *socket, m_connectionSet)
    {
        socket->close();
        socket->deleteLater();
    }
    m_tcpServer->close();
    m_tcpServer->deleteLater();
    // m_xmlReader->deleteLater();
    delete m_ui;
}

void MainWindow::newConnection()
{
    displayMessage(QString("%1 :: New message stream arriving").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    while(m_tcpServer->hasPendingConnections())
        appendToSocketList(m_tcpServer->nextPendingConnection());
}

void MainWindow::appendToSocketList(QTcpSocket *socket)
{
    m_connectionSet.insert(socket);
    connect(socket,&QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(socket,&QTcpSocket::disconnected, this, &MainWindow::deleteSocket);
}

void MainWindow::readSocket()
{
    qDebug() << "Reading " << sender();
    QTcpSocket *socket = reinterpret_cast<QTcpSocket*>(sender());
    int bytesAvailable = socket->bytesAvailable();
    QByteArray newData = socket->readAll();
    m_buffer.append(newData);
}

void MainWindow::deleteSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = m_connectionSet.find(socket);
    if (it != m_connectionSet.end()){
        displayMessage(QString("INFO :: A client has just left the room %1").arg(socket->socketDescriptor()));
        // qDebug() << QString("INFO :: A client has just left the room %1").arg(socket->socketDescriptor());
        m_connectionSet.remove(*it);
    }
    qDebug() << "Deleting " << sender();
    socket->deleteLater();
    doRegexMatch();
    m_buffer.clear();
}

void MainWindow::doRegexMatch()
{
    // qDebug() << m_buffer;
    QRegularExpressionMatchIterator matchIterator = m_regexPattern.globalMatch(m_buffer);
    while(matchIterator.hasNext()){
       QRegularExpressionMatch match = matchIterator.next();
       if(match.hasMatch()){
            QString matches = match.captured(0);
            m_regexMatches.append(matches);
       }
    }
}

void MainWindow::parseXmlString(int i)
{
    m_xmlReader->clear();
    if(m_regexMatches.isEmpty() || m_regexMatches.count() <= i) return;
    QString xmlString = m_regexMatches.at(i);
    m_xmlReader->addData(xmlString);
    QString LogEventType, LogEventID,AirStarttimeLocal, AirStoptimeLocal, AirDate,
    AssetID, AssetFileName, AssetTypeID, AssetTypeName, 
    ArtistID, ArtistName, AlbumID, AlbumName, Title, Comment,
    ParticipantID, ParticipantName, SponsorID, SponsorName, ProductID, ProductName, 
    RwLocal, RwCanCon, RwHit, RwFemale, RwIndigenous, RwExplicit, RwReleaseDate, RwGenre;

    bool hasAttributeLastStarted = false;
    bool hasAttributeValidLogEventID = false;
    bool hasTagAsset = false;
    bool hasTagTask = false;

    /**
     * @brief 
     * if isAsset && LogEventID > 0 
     *      if LastStarted==true
     *          <PrerecAsset/>
     *      else
     *          do nothing;
     * else if isAsset && LogEventID == 0
     *      return <WeirdLiveAsset/>
     * else if !isAsset && isTask
     *      <Task/>
     * else
     *      <LogEvent> -- can acquire StartTime,StopTime,try it's best but cant distinguish b/w PrerecAsset and LiveShow
     * 
     */

    while (!m_xmlReader->atEnd()) 
    {
        m_xmlReader->readNext();
        if(m_xmlReader->tokenType() == QXmlStreamReader::StartElement)
        {
            if(m_xmlReader->name() == "LogEvent")
            {
                LogEventID = m_xmlReader->attributes().value("LogEventID").toString();
                AirStarttimeLocal = m_xmlReader->attributes().value("AirStarttimeLocal").toString();
                AirStoptimeLocal = m_xmlReader->attributes().value("AirStoptimeLocal").toString();
                AirDate = AirStarttimeLocal.mid(0,10);
                hasAttributeLastStarted = m_xmlReader->attributes().hasAttribute("LastStarted");
                hasAttributeValidLogEventID = LogEventID.toInt();
            }
            else if(m_xmlReader->name() == "Asset")
            {
                hasTagAsset = 1;
                // if it's a stopping PrerecAsset, then break the while loop.
                if(!hasAttributeLastStarted && hasAttributeValidLogEventID)
                    break; 
                else
                {   
                    LogEventType = "Prerec. Asset";
                    AssetID = m_xmlReader->attributes().value("AssetID").toString();
                    AssetTypeID = m_xmlReader->attributes().value("AssetTypeID").toString();
                    AssetTypeName = m_xmlReader->attributes().value("AssetTypeName").toString();
                    Title = m_xmlReader->attributes().value("Title").toString();
                    Comment = m_xmlReader->attributes().value("Comment").toString();
                }
            }
            else if(m_xmlReader->name() == "Transition")
            {
                AssetFileName = m_xmlReader->attributes().value("Filename").toString();
            }
            else if(m_xmlReader->name() == "Artist")
            {
                ArtistName = m_xmlReader->attributes().value("Name").toString();
                ArtistID = m_xmlReader->attributes().value("ArtistID").toString();
            }
            else if(m_xmlReader->name() == "Album")
            {
                AlbumName = m_xmlReader->attributes().value("Name").toString();
                AlbumID = m_xmlReader->attributes().value("AlbumID").toString();
            }
            else if(m_xmlReader->name() == "Participant")
            {
                // if it's a PrerecAsset then Partipant Tag will contain Name and ID
                if(hasAttributeValidLogEventID && hasTagAsset)
                {
                    ParticipantName = m_xmlReader->attributes().value("Name").toString();
                    ParticipantID = m_xmlReader->attributes().value("ParticipantID").toString();
                }
                // if it's a WeirdAsset then Particpant Tag will contain only Name and this is the LogEventType
                else
                {
                    LogEventType = m_xmlReader->attributes().value("Name").toString();
                }
            }
            else if(m_xmlReader->name() == "Sponsor")
            {
                SponsorName = m_xmlReader->attributes().value("Name").toString();
                SponsorID = m_xmlReader->attributes().value("SponsorID").toString();
            }
            else if(m_xmlReader->name() == "AssetAttribute")
            {
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "Genre")
                    RwGenre = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "RW Release Date")
                    RwReleaseDate = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "Local")
                    RwLocal = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "CanCon")
                    RwCanCon = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "Hit")
                    RwHit = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "Explicit")
                    RwExplicit = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "Female")
                    RwFemale = m_xmlReader->attributes().value("AttributeValueName").toString();
                if(m_xmlReader->attributes().value("AttributeTypeName").toString() == "Indigenous")
                    RwIndigenous = m_xmlReader->attributes().value("AttributeValueName").toString();
            }
            else if(m_xmlReader->name() == "Task")
            {
                hasTagTask = 1;
                LogEventID = generateDefaultLogEventID();
                Comment = m_xmlReader->attributes().value("Comment").toString();
                if(Comment.contains("Sequencer.SetMode"))
                {
                    qDebug("Sequencer.SetMode Detected");
                    LogEventType = "Sequencer.SetMode";
                    if(Comment.contains("Live Assist"))
                        AssetTypeName = "Live Assist";
                    else if(Comment.contains("Auto"))
                        AssetTypeName = "Auto";
                }
                else if(Comment.contains("LiveMetadata.Send"))
                {
                    qDebug("LiveMetadata.Send Detected");
                    LogEventType = "LiveMetadata.Send";
                    qsizetype i1 = Comment.indexOf("Title: ");
                    qsizetype i2 = Comment.indexOf(", Artist: ");
                    qsizetype i3 = Comment.indexOf(", Composer: ");
                    qsizetype i4 = Comment.indexOf(" ]");
                    if(i1 != -1 && i2 != -1)
                        Title = Comment.mid(i1+7,i2-i1-7);
                    if(i2 != -1 && i3 != -1)
                        ArtistName = Comment.mid(i2+10,i3-i2-10);
                    if(i3 != -1 && i4 != -1)
                        AssetTypeName = Comment.mid(i3+12,i4-i3-12);
                }
            }
        }
        if (m_xmlReader->hasError()) 
            qDebug() << "ERROR: " << m_xmlReader->errorString();    
    }
    qDebug() << "\nLogEventType: " << LogEventType << "\nLogEventID: " << LogEventID << "\nAirStarttimeLocal: " << AirStarttimeLocal << "\nAirStoptimeLocal: " << AirStoptimeLocal << "\nAirDate: " << AirDate << "\nAssetID: " << AssetID << "\nAssetFileName: " << AssetFileName << "\nAssetTypeID: " << AssetTypeID << "\nAssetTypeName: " << AssetTypeName << "\nArtistID: " << ArtistID << "\nArtistName: " << ArtistName << "\nAlbumID: " << AlbumID << "\nAlbumName: " << AlbumName << "\nTitle: " << Title << "\nComment: " << Comment << "\nParticipantID: " << ParticipantID << "\nParticipantName: " << ParticipantName << "\nSponsorID: " << SponsorID << "\nSponsorName: " << SponsorName << "\nProductID: " << ProductID << "\nProductName: " << ProductName << "\nRwLocal: " << RwLocal << "\nRwCanCon: " << RwCanCon << "\nRwHit: " << RwHit << "\nRwFemale: " << RwFemale << "\nRwIndigenous: " << RwIndigenous << "\nRwExplicit: " << RwExplicit << "\nRwReleaseDate: " << RwReleaseDate << "\nRwGenre: " << RwGenre;
    qDebug() << "hasAttributeLastStarted " << hasAttributeLastStarted << " | " << "hasAttributeValidLogEventID " << hasAttributeValidLogEventID << " | " << "hasTagAsset " << hasTagAsset << " | " << "hasTagTask "  << hasTagTask;
}



void MainWindow::displayMessage(const QString& str)
{
    m_ui->textBrowser_receivedMessages->append(str);
}

void MainWindow::on_pushButton_sendMessage_clicked()
{
    // qDebug() << m_regexMatches;
    QString iString = m_ui->lineEdit_message->text();
    int i = iString.toInt();
    parseXmlString(i);
}

void MainWindow::on_pushButton_sendAttachment_clicked()
{
    m_ui->textBrowser_receivedMessages->clear();

}