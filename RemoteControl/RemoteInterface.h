#ifndef REMOTEINTERFACE_H
#define REMOTEINTERFACE_H

#include "RemoteCommand.h"

#include <QObject>
#include <QHostAddress>


namespace RemoteControl
{
class Server;

class RemoteInterface : public QObject
{
    Q_OBJECT
public:
    void sendImage(int index, const QImage& image);
    static RemoteInterface& instance();
    void sendImageCount(int count);

private slots:
    void handleCommand(const RemoteCommand&);
    void sendCategoryNames(const RequestCategoryInfo& searchInfo);
    void sendCategoryValues(const RequestCategoryInfo& searchInfo);

private:
    explicit RemoteInterface(QObject *parent = 0);
    Server* m_connection;
};

}
#endif // REMOTEINTERFACE_H
