#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QSet>
#include <QList>
#include <QPair>

class QPushButton;
class QDockWidget;
class QLabel;

namespace AnnotationDialog {

class ListSelect;
typedef QPair<QDockWidget*, QWidget*> DockPair;

/**
 * This class is to help set up the right shortcuts for the annotation dialog.
 * The are two problems with KDE's default shortcuts:
 * (1) There is a bug in Qt, so that shortcuts are actually not set up for
 * QDockWidgets. (This might be fixed when you read this, of course)
 * (2) KDE's shortcut handling does of course not know anything about the
 * annotation dialog, and may therefore not set up an optiomal set of
 * shortcuts.
 * This class ensures that each of the list selects (which are what the
 * user will use the most) will get shortcuts assigned first, and then the
 * other widgets will.
 */
class ShortCutManager
{
public:
    void addDock( QDockWidget* dock, QWidget* buddy );
    void addLabel( QLabel* label );
    void setupShortCuts();
    void addTaken( const QString& );

private:
    QList< DockPair > m_docks;
    QList< QLabel* > m_labelWidgets;
    QSet<QChar> m_taken;
};

}

#endif /* SHORTCUTMANAGER_H */

