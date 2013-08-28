#ifndef CLIENTFORM_H
#define CLIENTFORM_H

#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer>
#include <QStandardItemModel>
#include "../MsgType.h"
namespace Ui {
    class ClientForm;
}

class ClientForm : public QWidget
{
    Q_OBJECT

public:
    explicit ClientForm(QWidget *parent = 0);
    ~ClientForm();

private slots:
    void ON_Connect();
    void ON_Send();

    void ON_ReadyRead();
    void ON_SokcetError(QAbstractSocket::SocketError error);
    void SendMsg(QModelIndex & index);

    void EncodeCennectHoleMsg(ST_Msg &msg, QByteArray &data);

private:
    void GetUserList(QDataStream& stream);
    void MakeHole(QDataStream &stream);
    void DecodeHoleMsg(ST_Msg &msg, QDataStream &stream);
    void HandleListenReady(QDataStream &stream);
    Ui::ClientForm *ui;
    QTcpSocket* server_socket_;

    QTcpSocket* user_socket_;
    QTcpServer* hole_socket_;
    QStandardItemModel* mode_;
};

#endif // CLIENTFORM_H
