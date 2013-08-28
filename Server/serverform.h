#ifndef SERVERFORM_H
#define SERVERFORM_H

#include <QWidget>
#include <QTcpServer>
#include <QSqlTableModel>
#include <QHostAddress>
#include <QHash>
#include <QTcpSocket>
#include <QPointer>
#include "../MsgType.h"

namespace Ui {
    class ServerForm;
}



class ServerForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServerForm(QWidget *parent = 0);
    ~ServerForm();

private slots:
    void ON_NewConnection();
    void ON_Set();
    void ON_ReadyRead();
    void ON_SokcetError(QAbstractSocket::SocketError error);

private:
    void DecodeHoleMsg(ST_Msg &msg, QDataStream &stream);
    void EncodeHoleMsg(ST_Msg &msg, QByteArray &data);
    void ForwardListenReady(QDataStream &stream, QTcpSocket* socket);
    void SendUserList(QTcpSocket* socket);
    bool CloseOldUser(QString user_name);
    Ui::ServerForm *ui;
    QTcpServer* server_main_;
    QTcpServer* server_hole_socket_;
    QSqlTableModel* mode_;
    QSqlTableModel* mode_hide_;
    QHash <QString, QTcpSocket*> user_info;
};

#endif // SERVERFORM_H
