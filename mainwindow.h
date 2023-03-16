#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDateTime>
#include <QByteArray>
#include <QXmlStreamReader>
#include <QIODevice>
#include <QTcpSocket>
#include <QTcpServer>
#include <QSet>
#include <QAbstractSocket>
#include <QMainWindow>

namespace Ui {class MainWindow;}

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newConnection();
    void appendToSocketList(QTcpSocket*);
    void readSocket();
    void deleteSocket();
    void parseXmlString(int);
    void displayMessage(const QString& str);
    void doRegexMatch();
    void on_pushButton_sendMessage_clicked();
    void on_pushButton_sendAttachment_clicked();
    QString generateDefaultTimestamp(){return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ap");}
    QString generateDefaultDate(){return QDateTime::currentDateTime().toString("yyyy-MM-dd");}
    QString generateDefaultLogEventID(){return QDateTime::currentDateTime().toString("yyyyMMddhhmmss");}

private:
    Ui::MainWindow *m_ui;
    QTcpServer *m_tcpServer;
    QSet<QTcpSocket*> m_connectionSet;
    QXmlStreamReader *m_xmlReader;
    QByteArray m_buffer;
    QRegularExpression m_regexPattern;
    QVector<QString> m_regexMatches; // pieces from m_buffer that match the regex-pattern
};