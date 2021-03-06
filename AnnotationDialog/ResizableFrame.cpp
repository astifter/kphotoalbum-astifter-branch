/* Copyright (C) 2014 Tobias Leupold <tobias.leupold@web.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// The basic resizable QFrame has been shamelessly stolen from
// http://qt-project.org/forums/viewthread/24104
// Big thanks to Mr. kripton :-)

#include "ResizableFrame.h"

// Qt includes
#include <QMouseEvent>
#include <QMenu>
#include <QApplication>
#include <QList>
#include <QDebug>
#include <QTimer>

// KDE includes
#include <KLocale>

// Local includes
#include "ImagePreview.h"
#ifdef HAVE_KFACE
#  include "ProposedFaceDialog.h"
#endif

static const int SCALE_TOP    = 0b00000001;
static const int SCALE_BOTTOM = 0b00000010;
static const int SCALE_RIGHT  = 0b00000100;
static const int SCALE_LEFT   = 0b00001000;
static const int MOVE         = 0b10000000;

static const QString STYLE_UNASSOCIATED = QString::fromUtf8(
    "AnnotationDialog--ResizableFrame { color: rgb(255,0,0); }"
    "AnnotationDialog--ResizableFrame:hover { background-color: rgb(255,255,255,30); }"
);
static const QString STYLE_PROPOSED = QString::fromUtf8(
    "AnnotationDialog--ResizableFrame { color: rgb(255,255,0); }"
    "AnnotationDialog--ResizableFrame:hover { background-color: rgb(255,255,255,30); }"
);
static const QString STYLE_ASSOCIATED = QString::fromUtf8(
    "AnnotationDialog--ResizableFrame { color: rgb(0,255,0); }"
    "AnnotationDialog--ResizableFrame:hover { background-color: rgb(255,255,255,30); }"
);

AnnotationDialog::ResizableFrame::ResizableFrame(QWidget* parent) : QFrame(parent)
{
    m_preview = dynamic_cast<ImagePreview*>(parent);

#ifdef HAVE_KFACE
    m_previewWidget = dynamic_cast<ImagePreviewWidget *>(m_preview->parentWidget());

    // The area has not been changed yet
    m_changed = false;
    // The area has not be used to train the recognition database yet
    m_trained = false;
    // Until we are told otherwise, assume this area was drawn manually by the user.
    m_detectedFace = false;
    // When we're constructing, there's no proposed face dialog.
    m_proposedFaceDialog = 0;
#endif

    setFrameShape(QFrame::Box);
    setMouseTracking(true);
    setStyleSheet(STYLE_UNASSOCIATED);

    m_removeAct = new QAction(
        i18nc("area of an image; rectangle that is overlayed upon the image",
              "Remove area"), this
    );
    connect(m_removeAct, SIGNAL(triggered()), this, SLOT(remove()));

    m_removeTagAct = new QAction(this);
    connect(m_removeTagAct, SIGNAL(triggered()), this, SLOT(removeTag()));

#ifdef HAVE_KFACE
    m_updateRecognitionDatabaseAct = new QAction(this);
    connect(m_updateRecognitionDatabaseAct, SIGNAL(triggered()), this, SLOT(updateRecognitionDatabase()));

    m_recognizeAct = new QAction(i18n("Try to recognize this face"), this);
    connect(m_recognizeAct, SIGNAL(triggered()), this, SLOT(recognize()));
#endif
}

AnnotationDialog::ResizableFrame::~ResizableFrame()
{
}

void AnnotationDialog::ResizableFrame::setActualCoordinates(QRect actualCoordinates)
{
    m_actualCoordinates = actualCoordinates;
}

QRect AnnotationDialog::ResizableFrame::actualCoordinates() const
{
    return m_actualCoordinates;
}

void AnnotationDialog::ResizableFrame::getMinMaxCoordinates()
{
    // Get the maximal area to drag or resize the frame
    m_minMaxCoordinates = m_preview->minMaxAreaPreview();
    // Add one pixel (width of the frame)
    m_minMaxCoordinates.setWidth(m_minMaxCoordinates.width() + 1);
    m_minMaxCoordinates.setHeight(m_minMaxCoordinates.height() + 1);
}

void AnnotationDialog::ResizableFrame::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        m_dragStartGeometry = geometry();

        // Just in case this will be a drag/resize and not just a click
        getMinMaxCoordinates();
    }
}

void AnnotationDialog::ResizableFrame::mouseMoveEvent(QMouseEvent* event)
{
    static int moveAction = 0;
    if (! (event->buttons() & Qt::LeftButton)) {
        // No drag, just change the cursor and return
        if (event->x() <= 3 && event->y() <= 3) {
            moveAction = SCALE_TOP | SCALE_LEFT;
            setCursor(Qt::SizeFDiagCursor);
        } else if (event->x() <= 3 && event->y() >= height() - 3) {
            moveAction = SCALE_BOTTOM | SCALE_LEFT;
            setCursor(Qt::SizeBDiagCursor);
        } else if (event->x() >= width() - 3 && event->y() <= 3) {
            moveAction = SCALE_TOP | SCALE_RIGHT;
            setCursor(Qt::SizeBDiagCursor);
        } else if (event->x() >= width() - 3 && event->y() >= height() - 3) {
            moveAction = SCALE_BOTTOM | SCALE_RIGHT;
            setCursor(Qt::SizeFDiagCursor);
        } else if (event->x() <= 3) {
            moveAction = SCALE_LEFT;
            setCursor(Qt::SizeHorCursor);
        } else if (event->x() >= width() - 3) {
            moveAction = SCALE_RIGHT;
            setCursor(Qt::SizeHorCursor);
        } else if (event->y() <= 3) {
            moveAction = SCALE_TOP;
            setCursor(Qt::SizeVerCursor);
        } else if (event->y() >= height() - 3) {
            moveAction = SCALE_BOTTOM;
            setCursor(Qt::SizeVerCursor);
        } else {
            moveAction = MOVE;
            setCursor(Qt::SizeAllCursor);
        }
        return;
    }

    int x;
    int y;
    int w;
    int h;

    h = height();

    if (moveAction & MOVE) {
        x = m_dragStartGeometry.left() - (m_dragStartPosition.x() - event->x());
        y = m_dragStartGeometry.top() - (m_dragStartPosition.y() - event->y());
        w = width();

        // Be sure not to move out of the preview
        if (x < m_minMaxCoordinates.left()) {
            x = m_minMaxCoordinates.left();
        }
        if (y < m_minMaxCoordinates.top()) {
            y = m_minMaxCoordinates.top();
        }
        if (x + w > m_minMaxCoordinates.width()) {
            x = m_minMaxCoordinates.width() - w;
        }
        if (y + h > m_minMaxCoordinates.height()) {
            y = m_minMaxCoordinates.height() - h;
        }
    } else {
        // initialize with the "missing" values when only one direction is manipulated:
        x = m_dragStartGeometry.left();
        y = m_dragStartGeometry.top();
        w = m_dragStartGeometry.width();

        if (moveAction & SCALE_TOP) {
            y = m_dragStartGeometry.top() - (m_dragStartPosition.y() - event->y());

            if (y >= geometry().y() + geometry().height()) {
                h = 0;
                y = m_dragStartGeometry.top() + m_dragStartGeometry.height();
                moveAction ^= SCALE_BOTTOM | SCALE_TOP;
            }

            if(y < m_minMaxCoordinates.top()) {
                y = m_minMaxCoordinates.top();
                h = m_dragStartGeometry.top() + m_dragStartGeometry.height() - m_minMaxCoordinates.y();
            } else {
                h = height() + (m_dragStartPosition.y() - event->y());
            }
        } else if (moveAction & SCALE_BOTTOM) {
            y = m_dragStartGeometry.top();
            h = event->y();

            if (h <= 0) {
                h = 0;
                m_dragStartPosition.setY(0);
                moveAction ^= SCALE_BOTTOM | SCALE_TOP;
            }

            if (y + h > m_minMaxCoordinates.height()) {
                h = m_minMaxCoordinates.height() - y;
            }
        }

        if (moveAction & SCALE_RIGHT) {
            x = m_dragStartGeometry.left();
            w = event->x();

            if (w <= 0) {
                w = 0;
                m_dragStartPosition.setX(0);
                moveAction ^= SCALE_RIGHT | SCALE_LEFT;
            }

            if (x + w > m_minMaxCoordinates.width()) {
                w = m_minMaxCoordinates.width() - x;
            }
        } else if (moveAction & SCALE_LEFT) {
            x = m_dragStartGeometry.left() - (m_dragStartPosition.x() - event->x());

            if (x >= geometry().left() + geometry().width()) {
                w = 0;
                x = m_dragStartGeometry.left() + m_dragStartGeometry.width();
                moveAction ^= SCALE_RIGHT | SCALE_LEFT;
            }

            if (x < m_minMaxCoordinates.left()) {
                x = m_minMaxCoordinates.left();
                w = m_dragStartGeometry.x() + m_dragStartGeometry.width() - m_minMaxCoordinates.x();
            } else {
                w = m_dragStartGeometry.width() + (m_dragStartPosition.x() - event->x());
            }
        }
    }
    setGeometry(x, y, w, h);
    m_dragStartGeometry = geometry();

    if (! m_tagData.first.isEmpty()) {
        // If we change an area with an associated tag:
        // tell the Dialog we made a change that should be saved (set the dirty marker)
        m_dialog->areaChanged();
    }
}

void AnnotationDialog::ResizableFrame::checkGeometry()
{
    // If this is called when the area is created, we don't have it yet
    getMinMaxCoordinates();

    // First cache the current geometry
    int x;
    int y;
    int w;
    int h;
    x = geometry().x();
    y = geometry().y();
    w = geometry().width();
    h = geometry().height();

    // Be sure no non-visible area is created by resizing to a height or width of 0
    // A height and width of 3 is the minimum to have more than a line
    if (geometry().height() < 3) {
        y = y - 1;
        h = 3;
    }
    if (geometry().width() < 3) {
        x = x - 1;
        w = 3;
    }

    // Probably, the above tweaking moved the area out of the preview area
    if (x < m_minMaxCoordinates.left()) {
        x = m_minMaxCoordinates.left();
    }
    if (y < m_minMaxCoordinates.top()) {
        y = m_minMaxCoordinates.top();
    }
    if (x + w > m_minMaxCoordinates.width()) {
        x = m_minMaxCoordinates.width() - w;
    }
    if (y + h > m_minMaxCoordinates.height()) {
        y = m_minMaxCoordinates.height() - h;
    }

    // If anything has been changed, set the updated geometry
    if (geometry().x() != x
        || geometry().y() != y
        || geometry().width() != w
        || geometry().height() != h) {

        setGeometry(x, y, w, h);
    }
}

void AnnotationDialog::ResizableFrame::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        checkGeometry();
        setActualCoordinates(m_preview->areaPreviewToActual(geometry()));
    }
}

void AnnotationDialog::ResizableFrame::contextMenuEvent(QContextMenuEvent* event)
{
    // Create the context menu
    QMenu* menu = new QMenu(this);

    // Let's see if we already have an associated tag
    if (! m_tagData.first.isEmpty()) {
        m_removeTagAct->setText(
            i18nc("As in: remove tag %1 in category %2 [from this marked area of the image]",
                  "Remove tag %1 (%2)",
                  m_tagData.second,
                  m_tagData.first)
        );
        menu->addAction(m_removeTagAct);

    } else {
        // Handle the last selected positionable tag (if we have one)
        QPair<QString, QString> lastSelectedPositionableTag = m_dialog->lastSelectedPositionableTag();
        if (! lastSelectedPositionableTag.first.isEmpty()) {
            QAction* associateLastSelectedTagAction = createAssociateTagAction(
                lastSelectedPositionableTag,
                i18n("Associate with")
            );
            connect(associateLastSelectedTagAction, SIGNAL(triggered()), this, SLOT(associateTag()));
            menu->addAction(associateLastSelectedTagAction);
        }

        // Handle all positionable tag candidates

        QList<QPair<QString, QString>> positionableTagCandidates = m_dialog->positionableTagCandidates();
        // If we have a last selected positionable tag: remove it
        positionableTagCandidates.removeAt(positionableTagCandidates.indexOf(lastSelectedPositionableTag));

        // If we still have candidates:
        if (positionableTagCandidates.length() > 0) {
            if (positionableTagCandidates.length() == 1
                && lastSelectedPositionableTag.first.isEmpty()) {

                // Add a single action
                QAction* associateOnlyCandidateAction = createAssociateTagAction(
                    positionableTagCandidates[0],
                    i18n("Associate with")
                );
                connect(associateOnlyCandidateAction, SIGNAL(triggered()), this, SLOT(associateTag()));
                menu->addAction(associateOnlyCandidateAction);
            } else {
                // Create a new menu for all other tags
                QMenu* submenu = menu->addMenu(
                    i18nc("As in: associate [this marked area of the image] with one of the "
                          "following choices/menu items",
                          "Associate with")
                );

                for (const QPair<QString, QString>& tag : positionableTagCandidates) {
                    submenu->addAction(createAssociateTagAction(tag));
                }

                connect(submenu, SIGNAL(triggered(QAction*)), this, SLOT(associateTag(QAction*)));
            }
        }
    }

    menu->addSeparator();

#ifdef HAVE_KFACE
    if (m_tagData.first.isEmpty() &&  m_proposedTagData.first.isEmpty()) {
        // If we have nothing, offer a recognition database lookup
        menu->addAction(m_recognizeAct);
    }

    if (! m_tagData.first.isEmpty() && m_changed && ! m_trained) {
        // Append a "Update recognition database with this face" action
        m_updateRecognitionDatabaseAct->setText(
            i18n("Train the recognition database with the face of %1", m_tagData.second)
        );
        menu->addAction(m_updateRecognitionDatabaseAct);
    }
#endif

    // Append the "Remove area" action
    menu->addAction(m_removeAct);

    // Show the menu
    menu->exec(event->globalPos());

    // Clean up the menu
    delete menu;
}

QAction* AnnotationDialog::ResizableFrame::createAssociateTagAction(
    const QPair<QString, QString>& tag,
    QString prefix
)
{
    QString actionText;
    if (! prefix.isEmpty()) {
        actionText = i18nc("%1 is a prefix like 'Associate with', "
                           "%2 is the tag name and %3 is the tag's category",
                           "%1 %2 (%3)").arg(prefix, tag.second, tag.first);
    } else {
        actionText = i18nc("%1 is the tag name and %2 is the tag's category",
                           "%1 (%2)").arg(tag.second, tag.first);
    }

    QAction* action = new QAction(actionText, this);
    QStringList data;
    data << tag.first << tag.second;
    action->setData(data);

    return action;
}

void AnnotationDialog::ResizableFrame::associateTag()
{
    QAction* action = dynamic_cast<QAction*>(sender());
    setTagData(action->data().toStringList()[0], action->data().toStringList()[1]);
}

void AnnotationDialog::ResizableFrame::associateTag(QAction* action)
{
    setTagData(action->data().toStringList()[0], action->data().toStringList()[1]);
}

void AnnotationDialog::ResizableFrame::setTagData(QString category, QString tag, ChangeOrigin changeOrigin)
{
    // Add the data to this area
    QPair<QString, QString> selectedData = QPair<QString, QString>(category, tag);
    m_tagData = selectedData;

    // Update the tool tip
    setToolTip(tag + QString::fromUtf8(" (") + category + QString::fromUtf8(")"));

    // Set the color to "associated"
    setStyleSheet(STYLE_ASSOCIATED);

    // Remove the associated tag from the tag candidate list
    m_dialog->removeTagFromCandidateList(m_tagData.first, m_tagData.second);

#ifdef HAVE_KFACE
    // Check if the selected data is the data that has been (probably) proposed by face recognition
    if (selectedData == m_proposedTagData) {
        // Disable to offer training with this area (it has already been recognized correctly)
        m_trained = true;
    }

    // Check of other areas contain this tag as a proposed tag
    m_dialog->checkProposedTagData(m_tagData, this);

    // If this is a manual update, update m_changed so that
    // we can (probably) train the recognition database
    if (changeOrigin == ManualChange) {
        m_changed = true;

        if (m_detectedFace and ! m_trained and m_previewWidget->automatedTraining()) {
            m_preview->trainRecognitionDatabase(m_actualCoordinates, m_tagData);
            m_trained = true;
        }
    }
#endif

    if (changeOrigin != AutomatedChange) {
        // Tell the dialog an area has been changed
        m_dialog->areaChanged();
    }
}

void AnnotationDialog::ResizableFrame::removeTag()
{
    // Add the tag to the positionable candidate list again
    m_dialog->addTagToCandidateList(m_tagData.first, m_tagData.second);
    // Delete the tag data from this area
    removeTagData();
}

void AnnotationDialog::ResizableFrame::removeTagData()
{
    // Delete the data
    m_tagData.first.clear();
    m_tagData.second.clear();
    setToolTip(QString());

    // Set the color to "un-associated" or "proposed"
    if (m_proposedTagData.first.isEmpty()) {
        setStyleSheet(STYLE_UNASSOCIATED);
    } else {
        setStyleSheet(STYLE_PROPOSED);
    }

#ifdef HAVE_KFACE
    // Also reset the trained and changed state
    m_changed = false;
    m_trained = false;
#endif

    // Tell the dialog an area has been changed
    m_dialog->areaChanged();
}

void AnnotationDialog::ResizableFrame::remove()
{
    if (! m_tagData.first.isEmpty()) {
        // Re-add the associated tag to the candidate list
        removeTag();
    }

    // Delete the area
    this->deleteLater();
}

void AnnotationDialog::ResizableFrame::checkShowContextMenu()
{
    // Don't show the context menu when we don't have a last selected positionable tag
    if (m_dialog->lastSelectedPositionableTag().first.isEmpty()) {
        return;
    }

    // Show the context menu at the lower right corner of the newly created area
    QContextMenuEvent* event = new QContextMenuEvent(
        QContextMenuEvent::Mouse, QPoint(0, 0), QCursor::pos(), Qt::NoModifier
    );
    QApplication::postEvent(this, event);
}

void AnnotationDialog::ResizableFrame::setDialog(Dialog* dialog)
{
    m_dialog = dialog;
}

QPair<QString, QString> AnnotationDialog::ResizableFrame::tagData() const
{
    return m_tagData;
}

void AnnotationDialog::ResizableFrame::setProposedTagData(QPair<QString, QString> tagData)
{
    m_proposedTagData = tagData;
    setStyleSheet(STYLE_PROPOSED);
}

QPair<QString, QString> AnnotationDialog::ResizableFrame::proposedTagData() const
{
    return m_proposedTagData;
}

void AnnotationDialog::ResizableFrame::removeProposedTagData()
{
    m_proposedTagData = QPair<QString, QString>();
    setStyleSheet(STYLE_UNASSOCIATED);
    setToolTip(QString());
}

#ifdef HAVE_KFACE
void AnnotationDialog::ResizableFrame::acceptTag()
{
    // Be sure that the proposed tag is selected and update this area's tag information
    m_preview->acceptProposedTag(m_proposedTagData, this);

    // Tell the dialog an area has been changed
    m_dialog->areaChanged();
}

void AnnotationDialog::ResizableFrame::updateRecognitionDatabase()
{
    m_preview->trainRecognitionDatabase(m_actualCoordinates, m_tagData);
    m_trained = true;
}

void AnnotationDialog::ResizableFrame::recognize()
{
    m_preview->recognizeArea(this);
}

void AnnotationDialog::ResizableFrame::markAsFace()
{
    m_detectedFace = true;
}

void AnnotationDialog::ResizableFrame::enterEvent(QEvent*)
{
    if (! m_proposedTagData.first.isEmpty()
        && m_tagData.first.isEmpty()
        && m_proposedFaceDialog == 0) {

        m_proposedFaceDialog = new ProposedFaceDialog(this);
    }
}

void AnnotationDialog::ResizableFrame::leaveEvent(QEvent*)
{
    QTimer::singleShot(0, m_proposedFaceDialog, SLOT(checkUnderMouse()));
}

void AnnotationDialog::ResizableFrame::checkUnderMouse()
{
    if (! underMouse()) {
        m_proposedFaceDialog->deleteLater();
        m_proposedFaceDialog = 0;
    }
}

void AnnotationDialog::ResizableFrame::proposedFaceDialogRemoved()
{
    m_proposedFaceDialog = 0;
}
#endif

// vi:expandtab:tabstop=4 shiftwidth=4:
