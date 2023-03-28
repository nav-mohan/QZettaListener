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
    m_tcpLoggerConnection = new QTcpSocket(); // connects to ZettaLogge
    connect(m_tcpServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    connect(m_tcpLoggerConnection, &QAbstractSocket::connected, this, &MainWindow::loggerConnected);
    connect(m_tcpLoggerConnection, &QAbstractSocket::disconnected, this, &MainWindow::loggerDisconnected);
    connect(m_tcpLoggerConnection, &QAbstractSocket::errorOccurred, this, &MainWindow::loggerErrorOccurred);

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
    appendLog("New message arriving");
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
        appendLog(QString("Client %1 disconnected").arg(socket->socketDescriptor()));
        m_connectionSet.remove(*it);
    }
    qDebug() << "Deleting " << sender();
    socket->deleteLater();
    doRegexMatch();
    m_buffer.clear();
    appendLog(QString("Received %1 Logs").arg(m_regexMatches.size()));
    QVector <QMap<QString,QString>>deliverRecords;
    for (int i = 0; i < m_regexMatches.count(); i++)
    {
        QMap<QString,QString> m = parseXmlString(i);
        deliverRecords.push_back(m);
    }
    m_regexMatches.clear();
    sendData(deliverRecords);
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

QMap<QString,QString> MainWindow::parseXmlString(int i)
{
    QMap<QString,QString> map;
    if(m_regexMatches.isEmpty() || m_regexMatches.count() <= i) return map;
    m_xmlReader->clear();
    QString xmlString = m_regexMatches.at(i);
    m_xmlReader->addData(xmlString);
    QString LogEventType, LogEventID,AirStarttime, AirStoptime, AirDate,
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
                AirStarttime = m_xmlReader->attributes().value("AirStarttime").toString();
                AirStoptime = m_xmlReader->attributes().value("AirStoptime").toString();
                AirDate = AirStarttime.mid(0,10);
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
    qDebug() << "\nLogEventType: " << LogEventType << "\nLogEventID: " << LogEventID << "\nAirStarttime: " << AirStarttime << "\nAirStoptime: " << AirStoptime << "\nAirDate: " << AirDate << "\nAssetID: " << AssetID << "\nAssetFileName: " << AssetFileName << "\nAssetTypeID: " << AssetTypeID << "\nAssetTypeName: " << AssetTypeName << "\nArtistID: " << ArtistID << "\nArtistName: " << ArtistName << "\nAlbumID: " << AlbumID << "\nAlbumName: " << AlbumName << "\nTitle: " << Title << "\nComment: " << Comment << "\nParticipantID: " << ParticipantID << "\nParticipantName: " << ParticipantName << "\nSponsorID: " << SponsorID << "\nSponsorName: " << SponsorName << "\nProductID: " << ProductID << "\nProductName: " << ProductName << "\nRwLocal: " << RwLocal << "\nRwCanCon: " << RwCanCon << "\nRwHit: " << RwHit << "\nRwFemale: " << RwFemale << "\nRwIndigenous: " << RwIndigenous << "\nRwExplicit: " << RwExplicit << "\nRwReleaseDate: " << RwReleaseDate << "\nRwGenre: " << RwGenre;
    qDebug() << "hasAttributeLastStarted " << hasAttributeLastStarted << " | " << "hasAttributeValidLogEventID " << hasAttributeValidLogEventID << " | " << "hasTagAsset " << hasTagAsset << " | " << "hasTagTask "  << hasTagTask;

    map["LogEventType"] = LogEventType;
    map["LogEventID"] = LogEventID;
    map["AirStarttime"] = AirStarttime;
    map["AirStoptime"] = AirStoptime;
    map["AirDate"] = AirDate;
    map["AssetID"] = AssetID;
    map["AssetFileName"] = AssetFileName;
    map["AssetTypeID"] = AssetTypeID;
    map["AssetTypeName "] = AssetTypeName ;
    map["ArtistID"] = ArtistID;
    map["ArtistName"] = ArtistName;
    map["AlbumID"] = AlbumID;
    map["AlbumName"] = AlbumName;
    map["Title"] = Title;
    map["Comment"] = Comment;
    map["ParticipantID"] = ParticipantID;
    map["ParticipantName"] = ParticipantName;
    map["SponsorID"] = SponsorID;
    map["SponsorName"] = SponsorName;
    map["ProductID"] = ProductID;
    map["ProductName"] = ProductName;
    map["RwLocal"] = RwLocal;
    map["RwCanCon"] = RwCanCon;
    map["RwHit"] = RwHit;
    map["RwFemale"] = RwFemale;
    map["RwIndigenous"] = RwIndigenous;
    map["RwExplicit"] = RwExplicit;
    map["RwReleaseDate"] = RwReleaseDate;
    map["RwGenre"] = RwGenre;


    // on_pushButton_connectToZettaLogger_clicked();
    return (map);
}



void MainWindow::appendLog(const QString& str)
{
    m_ui->textBrowser_receivedMessages->append(QString("%1 ::%2")
    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
    .arg(str));

}

void MainWindow::on_pushButton_clearLogs_clicked()
{
    m_ui->textBrowser_receivedMessages->clear();
}

void MainWindow::on_pushButton_establishTcpServer_clicked()
{
    int portNumber = m_ui->zettaConnectionPortNumberValue->text().toInt();
    if(portNumber == 0) portNumber = 10000;
    if(m_tcpServer->listen(QHostAddress::AnyIPv4, portNumber))
    {
        qDebug("LISTENING...%d",portNumber);
        m_ui->statusBar->showMessage(QString("Server is listening on port %1").arg(portNumber));
    }
}

void MainWindow::on_pushButton_connectToZettaLogger_clicked()
{
    QString ipAddressPort = m_ui->loggerConnectionIpAddressPortValue->text();
    if(ipAddressPort == "") ipAddressPort="127.0.0.1:10001";
    qDebug() << "Connecting to zettaLogger" << ipAddressPort;
    QString ip = ipAddressPort.split(":").at(0);
    quint16 port = ipAddressPort.split(":").at(1).toInt();
    if(port == 0) port = 10000;
    m_tcpLoggerConnection->connectToHost(ip,port);
}

void MainWindow::loggerConnected()
{
    appendLog("Connected to ZettaLogger Server");
}

void MainWindow::loggerDisconnected()
{
    appendLog("Connection to ZettaLogger Server has closed");
}

void MainWindow::loggerErrorOccurred()
{
    appendLog(m_tcpLoggerConnection->errorString());
}


void MainWindow::sendData(QVector<QMap<QString,QString>> map)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    // Serialize the QVector<QMap<QString,QString>>
    stream << map;
    on_pushButton_connectToZettaLogger_clicked();
    // Send the serialized map over the socket
    m_tcpLoggerConnection->open(QIODevice::WriteOnly);
    m_tcpLoggerConnection->write(data);
    m_tcpLoggerConnection->close();

}

