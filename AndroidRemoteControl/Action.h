#ifndef REMOTECONTROL_ACTION_H
#define REMOTECONTROL_ACTION_H

#include <QString>
#include "RemoteCommand.h"
#include "Types.h"

namespace RemoteControl {

class Action
{
public:
    Action(const SearchInfo& searchInfo);
    void run();

protected:
    virtual void execute() = 0;
    void setCurrentPage(Page page);
    void sendCommand(const RemoteCommand& command);
    void clearCategoryModel();
    SearchInfo m_searchInfo;
};

class ShowOverviewAction :public Action
{
public:
    ShowOverviewAction(const SearchInfo& searchInfo);
protected:
    void execute() override;
};

class ShowCategoryValueAction :public Action
{
public:
    ShowCategoryValueAction(const SearchInfo& searchInfo, CategoryViewType type);
protected:
    void execute() override;
private:
    CategoryViewType m_type;
};



class ShowThumbnailsAction :public Action
{
public:
    ShowThumbnailsAction(const SearchInfo& searchInfo);
protected:
    void execute() override;
};


class ShowImagesAction :public Action
{
public:
    ShowImagesAction(int imageId, const SearchInfo& searchInfo);
protected:
    void execute() override;
private:
    const int m_imageId;
};

class DiscoverAction :public Action
{
public:
    DiscoverAction(const SearchInfo& searchInfo);

protected:
    void execute() override;
};

class ShowDiscoveredImage :public Action
{
public:
    ShowDiscoveredImage(ImageId imageId) : Action({}), m_imageId(imageId) {}

protected:
    void execute() override;

private:
    ImageId m_imageId;
};

} // namespace RemoteControl

#endif // REMOTECONTROL_ACTION_H
