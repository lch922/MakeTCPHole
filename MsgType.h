#pragma once
#include <QString>
// 服务器地址和端口号定义
#define SRV_TCP_MAIN_PORT		4000				// 服务器主连接的端口号
#define SRV_TCP_HOLE_PORT		8000				// 服务器主连接的端口号
#define NET_BUFFER_SIZE			1024				// 缓冲大小
// 数据包类型
typedef enum _packet_type
{
    PACKET_TYPE_INVALID = 0,
    PACKET_TYPE_NEW_USER_LOGIN,			// 服务器收到新的客户端登录，将登录信息发送给其他客户端
    PACKET_TYPE_WELCOME,				// 客户端登录时服务器发送该欢迎信息给客户端，以告知客户端登录成功
    PACKET_TYPE_REQUEST_CONN_CLIENT,	// 某客户端向服务器申请，要求与另一个客户端建立直接的TCP连接，即需要进行TCP打洞
    PACKET_TYPE_REQUEST_MAKE_HOLE,		// 服务器请求某客户端向另一客户端进行TCP打洞，即向另一客户端指定的外部IP和端口号进行connect尝试
    PACKET_TYPE_REQUEST_DISCONNECT,		// 请求服务器断开连接
    PACKET_TYPE_TCP_DIRECT_CONNECT,		// 服务器要求主动端（客户端A）直接连接被动端（客户端B）的外部IP和端口号
    PACKET_TYPE_HOLE_LISTEN_READY,		// 被动端（客户端B）打洞和侦听均已准备就绪
    PACKET_TYPE_Logon,
    PACKET_TYPE_UserList,
    PACKET_TYPE_CONNECT_ROUTE_PORT_ERROR,
    PACKET_TYPE_CONNECT_PC_PORT_ERROR,
    PACKET_TYPE_MAX
} PACKET_TYPE;

struct ST_Msg
{
    //消息类型
    PACKET_TYPE type;
    //当前用户
    QString user;
    //目标用户
    QString to_user;
    //远端IP
    QString remote_ip;
    //本机IP
    QString local_ip;
    //路由器出口端口
    quint16 route_out_port;
    //路由器入口端口
    quint16 route_in_port;
    //PC端口
    quint16 remote_port;
};
