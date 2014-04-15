#ifndef REMOTECONTROL_THUMBNAILMODEL_H
#define REMOTECONTROL_THUMBNAILMODEL_H

#include <QAbstractListModel>

namespace RemoteControl {

using RoleMap = QHash<int, QByteArray>;
class ThumbnailModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ThumbnailModel(QObject *parent = 0);
    enum { ImageIdRole };
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    RoleMap roleNames() const override;
    void setImages(const QList<int>&image);
    int indexOf(int imageId);

private:
    QList<int> m_images;

signals:

public slots:

};

} // namespace RemoteControl

Q_DECLARE_METATYPE(RemoteControl::ThumbnailModel*);

#endif // REMOTECONTROL_THUMBNAILMODEL_H
