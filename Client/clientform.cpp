#include <winsock.h>
#include "../MsgType.h"
#include "clientform.h"
#include "ui_clientform.h"

extern char* sz_msg[PACKET_TYPE_MAX] = {    "PACKET_TYPE_INVALID",
                                     "PACKET_TYPE_NEW_USER_LOGIN",			// 服务器收到新的客户端登录，将登录信息发送给其他客户端
                                     "PACKET_TYPE_WELCOME",				// 客户端登录时服务器发送该欢迎信息给客户端，以告知客户端登录成功
                                     "PACKET_TYPE_REQUEST_CONN_CLIENT",	// 某客户端向服务器申请，要求与另一个客户端建立直接的TCP连接，即需要进行TCP打洞
                                     "PACKET_TYPE_REQUEST_MAKE_HOLE",		// 服务器请求某客户端向另一客户端进行TCP打洞，即向另一客户端指定的外部IP和端口号进行connect尝试
                                     "PACKET_TYPE_REQUEST_DISCONNECT",		// 请求服务器断开连接
                                     "PACKET_TYPE_TCP_DIRECT_CONNECT",		// 服务器要求主动端（客户端A）直接连接被动端（客户端B）的外部IP和端口号
                                     "PACKET_TYPE_HOLE_LISTEN_READY",		// 被动端（客户端B）打洞和侦听均已准备就绪
                                     "PACKET_TYPE_Logon",
                                     "PACKET_TYPE_UserList"};

ClientForm::ClientForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClientForm)
{
    ui->setupUi(this);
    connect(ui->Connect, SIGNAL(clicked()), this, SLOT(ON_Connect()));
    server_socket_ = new QTcpSocket(this);
    connect(server_socket_, SIGNAL(readyRead()), this, SLOT(ON_ReadyRead()));
    connect(server_socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(ON_SokcetError(QAbstractSocket::SocketError)));
    user_socket_ = new QTcpSocket(this);
    connect(user_socket_, SIGNAL(readyRead()), this, SLOT(ON_ReadyRead()));
    connect(user_socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(ON_SokcetError(QAbstractSocket::SocketError)));
    mode_ = new QStandardItemModel(this);
    hole_socket_ = new QTcpServer(this);
    QStringList headers;
    headers << "User" << "IP" << "PORT";
    mode_->setHorizontalHeaderLabels(headers);
    ui->UserTab->setModel(mode_);
    ui->UserTab->setSelectionBehavior(QAbstractItemView::SelectRows);
}

ClientForm::~ClientForm()
{
    delete ui;
}

void ClientForm::ON_Connect()
{
    server_socket_->abort();
    server_socket_->connectToHost(ui->Server->text(), ui->Port->value());
    if (server_socket_->waitForConnected()) {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << PACKET_TYPE_Logon;
        stream << ui->User->text();
        server_socket_->write(data);
    }
}

void ClientForm::ON_Send()
{
}

void ClientForm::ON_ReadyRead()
{
    qDebug() << "ClientForm::ON_ReadyRead()";
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (NULL != socket){
        QByteArray data = socket->readAll();
        QDataStream stream(&data, QIODevice::ReadOnly);
        int type;
        stream >> type;
        qDebug() << "type" << sz_msg[type];
        switch ( type )
        {
            case PACKET_TYPE_WELCOME:
            {
                // 收到服务器的欢迎信息，说明登录已经成功
                ui->Msg->append("log in!");
                break;
            }
            case PACKET_TYPE_REQUEST_CONN_CLIENT: {
                //收到其他客户端要求打洞的请求
                MakeHole(stream);
            }
            case PACKET_TYPE_NEW_USER_LOGIN:
            {
                // 其他客户端（客户端B）登录到服务器了
                break;
            }
            case PACKET_TYPE_HOLE_LISTEN_READY: {
                //对方已准备好
                HandleListenReady(stream);
                break;
            }
            case PACKET_TYPE_REQUEST_MAKE_HOLE:
            {
                // 服务器要我（客户端B）向另外一个客户端（客户端A）打洞
                break;
            }
            case PACKET_TYPE_UserList:
            {
                GetUserList(stream);
                break;
            }
        }
    }
}

void ClientForm::HandleListenReady(QDataStream &stream)
{
    ST_Msg msg;
    DecodeHoleMsg(msg, stream);
    user_socket_->abort();
    user_socket_->connectToHost(msg.remote_ip, msg.remote_port);
    if (user_socket_->waitForConnected()) {
        //打洞成功
//        user_socket_->
    }
}

void ClientForm::MakeHole(QDataStream &stream)
{
    ST_Msg msg;
    DecodeHoleMsg(msg, stream);
    //向目标端口发送一条消息，让本端的NAT设备记录
    user_socket_->connectToHost(msg.remote_ip, msg.route_out_port);
    if (user_socket_->waitForConnected()) {
        //连接成功，表示打洞完成
        user_socket_->write("Hello i'm");
        user_socket_->write(ui->User->text().toUtf8().data());
    } else {
        //连接失败，对方的NAT设备为对称NAT
        //连接到服务器，如果本端的NAT设备不是对称NAT设备，则另一方的客户端连接过来的时候，是可以连接成功的
        msg.type = PACKET_TYPE_HOLE_LISTEN_READY;
        QString to_user = msg.to_user;
        msg.to_user = msg.user;
        msg.user = to_user;
        QByteArray data;
        EncodeCennectHoleMsg(msg, data);
        user_socket_->connectToHost(ui->Server->text(), ui->Port->value());
        if (user_socket_->waitForConnected()) {
            int flag = 1;
            setsockopt(user_socket_->socketDescriptor(),
                SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
            hole_socket_->listen(QHostAddress::Any, user_socket_->localPort());
            user_socket_->write(data);
        }
    }
}

void ClientForm::ON_SokcetError(QAbstractSocket::SocketError error)
{
}

void ClientForm::SendMsg(QModelIndex &index)
{
    hole_socket_->abort();
    hole_socket_->connectToHost(ui->Server->text(), SRV_TCP_HOLE_PORT);
    if (hole_socket_->waitForConnected()) {
        int flag = 1;
        setsockopt(hole_socket_->socketDescriptor(), SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
        ST_Msg msg;
        msg.type = PACKET_TYPE_REQUEST_CONN_CLIENT;
        msg.user = ui->User->text();
        msg.route_out_port = ui->OutPort->value();
        msg.route_in_port = ui->InPort->value();
        msg.to_user = mode_->data(mode_->index(index.row(), 0)).toString();
        QByteArray send_data;
        EncodeCennectHoleMsg(msg, send_data);
        hole_socket_->write(send_data);
    } else {
        qDebug() << "SendMsg" << hole_socket_->errorString();
    }
}

void ClientForm::DecodeHoleMsg(ST_Msg &msg, QDataStream &stream)
{
    stream >> msg.user;
    stream >> msg.to_user;
    stream >> msg.remote_ip;
    stream >> msg.local_ip;
    stream >> msg.route_out_port;
    stream >> msg.route_in_port;
    stream >> msg.remote_port;
}

void ClientForm::EncodeCennectHoleMsg(ST_Msg &msg, QByteArray &data)
{
    QDataStream send_stream(&data, QIODevice::WriteOnly);
    send_stream << msg.type;
    send_stream << msg.user;
    send_stream << msg.to_user;
    send_stream << msg.route_out_port;
    send_stream << msg.route_in_port;
    send_stream << msg.remote_port;
}

void ClientForm::GetUserList(QDataStream& stream)
{
    mode_->clear();
    QStringList ips;
    QList<quint16> ports;
    QStringList users;
    stream >> users;
    stream >> ips;
    stream >> ports;
    qDebug() << "users" << users << "ips" << ips << "ports" << ports;
    QStringList headers;
    headers << "User" << "IP" << "PORT";
    mode_->setHorizontalHeaderLabels(headers);
    mode_->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        mode_->setData(mode_->index(i, 0), users[i]);
        mode_->setData(mode_->index(i, 1), ips[i]);
        mode_->setData(mode_->index(i, 2), ports[i]);
    }
//    mode_->setRowCount(users.size());

    ui->UserTab->resizeColumnsToContents ();
    ui->UserTab->resizeRowsToContents ();

    QHeaderView *pTableHeaderView = ui->UserTab->horizontalHeader();
    if (pTableHeaderView) {
        pTableHeaderView->setDefaultAlignment(Qt::AlignCenter); //居左
        pTableHeaderView->setTextElideMode(Qt::ElideRight); //...效果
        pTableHeaderView->setStretchLastSection(true); //尾不留空白
    }
    ui->UserTab->setUpdatesEnabled(true);
}
