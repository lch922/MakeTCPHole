#include "serverform.h"
#include "ui_serverform.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDataStream>
#include <QByteArray>

void InitDB()
{
    QSqlDatabase dbconn = QSqlDatabase::addDatabase("QSQLITE");
    //当前目录下的test.db3数据库文件
    dbconn.setDatabaseName(":memory:");
    if(!dbconn.open()) {
        qDebug ("[%s]",dbconn.lastError().text().toUtf8().data());
        return;
    }

    QSqlQuery query;
    //UserTab
    query.exec("create table UserTab (User vchar(10) primary key, IP vchar(16), Port INTEGER)");
}

ServerForm::ServerForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServerForm)
{
    ui->setupUi(this);
    InitDB();

    server_main_ = new QTcpServer(this);
    connect(server_main_, SIGNAL(newConnection()), this, SLOT(ON_NewConnection()));

    server_hole_socket_ = new QTcpServer(this);
    connect(server_hole_socket_, SIGNAL(newConnection()), this, SLOT(ON_NewConnection()));

    connect(ui->Set, SIGNAL(clicked()), this, SLOT(ON_Set()));
    mode_ = new QSqlTableModel(this);
    mode_->setTable("UserTab");
    mode_hide_ = new QSqlTableModel(this);
    mode_hide_->setTable(mode_->tableName());
    QStringList header;
    header << "User" << "IP" << "Port";
    for (int i = 0; i < header.size(); ++i) {
        mode_->setHeaderData(i, Qt::Horizontal, header.at(i));
    }
    ui->UserTab->setModel(mode_);
}

ServerForm::~ServerForm()
{
    delete ui;
}

void ServerForm::ON_NewConnection()
{
    QTcpServer *send = qobject_cast<QTcpServer*>(sender());
    if (NULL != send){
        QTcpSocket* socket = server_main_->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(ON_ReadyRead()));
        connect(socket, SIGNAL(destroyed()), socket, SLOT(deleteLater()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(ON_SokcetError(QAbstractSocket::SocketError)));
    }
}

void ServerForm::ON_Set()
{
    server_main_->close();
    server_main_->listen(QHostAddress::Any, ui->Port->value());
    server_hole_socket_->close();
    server_hole_socket_->listen(QHostAddress::Any, SRV_TCP_HOLE_PORT);
}

void ServerForm::ON_ReadyRead()
{
    qDebug() << "ON_ReadyRead" ;
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (NULL != socket){
        QByteArray data = socket->readAll();
        QDataStream stream(&data, QIODevice::ReadOnly);
        int type;
        stream >> type;
        switch ( type )
            {
            case PACKET_TYPE_REQUEST_CONN_CLIENT:
            {
                // 要求与另一个客户端建立直接的TCP连接
                ST_Msg msg;
                DecodeHoleMsg(msg, stream);
                if (user_info.contains(msg.to_user)) {
                    QByteArray send_data;
                    msg.type = PACKET_TYPE_REQUEST_CONN_CLIENT;
                    msg.remote_ip = socket->peerAddress().toString();
                    msg.route_out_port= socket->peerPort();
                    EncodeHoleMsg(msg, send_data);
                    user_info[msg.to_user]->write(send_data);
                }
                break;
            }
            case PACKET_TYPE_REQUEST_DISCONNECT:
            {
                // 被动端（客户端B）请求服务器断开连接，这个时候应该将客户端B的外部IP和端口号告诉客户端A，并让客户端A主动
                // 连接客户端B的外部IP和端口号
                break;
            }
            case PACKET_TYPE_HOLE_LISTEN_READY:
            {
                // 被动端,打洞和侦听均已准备就绪
                ForwardListenReady(stream, socket);
                break;
            }
            case PACKET_TYPE_Logon:
            {
                SendUserList(socket);

                QString user;
                stream >> user;
                QSqlRecord record = mode_->record();
                record.setValue("User", user);
                record.setValue("IP", socket->peerAddress().toString());
                record.setValue("Port", socket->peerPort());
                mode_->insertRecord(mode_->rowCount(), record);
                //todo 强制旧用户退出
                user_info[user] = socket;
                break;
            }
            }
    }
}

void ServerForm::ForwardListenReady(QDataStream &stream, QTcpSocket* socket)
{
    ST_Msg msg;
    DecodeHoleMsg(msg, stream);
    if (user_info.contains(msg.to_user)) {
        msg.remote_port = socket->peerPort();
        msg.remote_ip = socket->peerAddress().toString();
        msg.type = PACKET_TYPE_HOLE_LISTEN_READY;
        QByteArray data;
        EncodeHoleMsg(msg, data);
        user_info[msg.to_user]->write(data);
    }
}

void ServerForm::DecodeHoleMsg(ST_Msg &msg, QDataStream &stream)
{
    stream >> msg.user;
    stream >> msg.to_user;
    stream >> msg.remote_ip;
    stream >> msg.local_ip;
    stream >> msg.route_out_port;
    stream >> msg.route_in_port;
    stream >> msg.remote_port;
}

void ServerForm::EncodeHoleMsg(ST_Msg &msg, QByteArray &data)
{
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << msg.type;
    stream << msg.user;
    stream << msg.to_user;
    stream << msg.remote_ip;
    stream << msg.local_ip;
    stream << msg.route_out_port;
    stream << msg.route_in_port;
    stream << msg.remote_port;
}

void ServerForm::ON_SokcetError(QAbstractSocket::SocketError error)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (NULL != socket) {
        socket->socketDescriptor();
    }
}

void ServerForm::SendUserList(QTcpSocket *socket)
{
    QStringList users;
    for (int i = 0; i < mode_->rowCount(); ++i) {
        users << mode_->data(mode_->index(i, 0)).toString();
    }
    QByteArray send_data;
    QDataStream send_stream(&send_data, QIODevice::WriteOnly);
    send_stream << PACKET_TYPE_UserList;
    send_stream << users;

    QStringList ips;
    QList<quint16> ports;
    QHash<QString, QTcpSocket*>::const_iterator i = user_info.begin();
     while (i != user_info.end()) {
         ips << i.value()->peerAddress().toString();
         ports << i.value()->peerPort();
         ++i;
     }
    send_stream << ips;
    send_stream << ports;
    socket->write(send_data);
}

bool ServerForm::CloseOldUser(QString user_name)
{
    if (user_info.contains(user_name)) {
        user_info[user_name]->close();
        return true;
    }
    return false;
}
