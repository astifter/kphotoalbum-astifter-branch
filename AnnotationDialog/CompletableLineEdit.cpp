#include "CompletableLineEdit.h"
#include <qregexp.h>
#include <qlistview.h>
#include <qapplication.h>


AnnotationDialog::CompletableLineEdit::CompletableLineEdit( ListSelect* parent, const char* name )
    :QLineEdit( parent, name )
{
    _listSelect = parent;
}

void AnnotationDialog::CompletableLineEdit::setListView( QListView* listbox )
{
    _listbox = listbox;
}

void AnnotationDialog::CompletableLineEdit::setMode( ListSelect::Mode mode )
{
    _mode = mode;
}

// Better hoope this monster works....
void AnnotationDialog::CompletableLineEdit::keyPressEvent( QKeyEvent* ev )
{
    if ( ev->key() == Key_Down || ev->key() == ev->Key_Up ) {
        selectPrevNextMatch( ev->key() == Key_Down );
        return;
    }

    if ( ev->key() == Key_Return ) {
        QLineEdit::keyPressEvent( ev );
        _listSelect->rePopulate();
        showOnlyItemsMatching( QString::null ); // Show all again
        return;
    }

    if ( ev->text().isEmpty() || !ev->text()[0].isPrint() ) {
        QLineEdit::keyPressEvent( ev );
        return;
    }

    // Don't insert the special character.
    if ( _mode == ListSelect::INPUT && isSpecialKey( ev ) )  {
        return;
    }

    // &,|, or ! should result in the current item being inserted
    if ( _mode == ListSelect::SEARCH && isSpecialKey( ev ) )  {
        handleSpecialKeysInSearch( ev );
        showOnlyItemsMatching( QString::null ); // Show all again after a special caracter.
        return;
    }

    QString content = text();
    int cursorPos = cursorPosition();
    int selStart = selectionStart();

    QLineEdit::keyPressEvent( ev );


    // Find the text of the current item
    int itemStart = 0;
    QString input = text();
    if ( _mode == ListSelect::SEARCH )  {
        input = input.left( cursorPosition() );
        itemStart = input.findRev( QRegExp(QString::fromLatin1("[!&|]")) ) +1;
        input = input.mid( itemStart );
    }

    // Find the text in the listbox
    QListViewItem* item = findItemInListView( input );
    if ( !item && _mode == ListSelect::SEARCH )  {
        // revert
        setText( content );
        setCursorPosition( cursorPos );
        item = findItemInListView( input );
        setSelection( selStart, content.length() ); // Reset previous selection.
    }

    if ( item )
        selectItemAndUpdateLineEdit( item, itemStart, input );

    showOnlyItemsMatching( input );
}

/**
 * Search for the first item in the appearance order, which matches text.
 */
QListViewItem* AnnotationDialog::CompletableLineEdit::findItemInListView( const QString& text )
{
    for ( QListViewItemIterator itemIt( _listbox ); *itemIt; ++itemIt ) {
        if ( itemMatchesText( *itemIt, text ) )
            return *itemIt;
    }
    return 0;
}

bool AnnotationDialog::CompletableLineEdit::itemMatchesText( QListViewItem* item, const QString& text )
{
    return item->text(0).lower().startsWith( text.lower() );
}

bool AnnotationDialog::CompletableLineEdit::isSpecialKey( QKeyEvent* ev )
{
    return ( ev->text() == QString::fromLatin1("&") ||
             ev->text() == QString::fromLatin1("|") ||
             ev->text() == QString::fromLatin1("!")
             /* || ev->text() == "(" */
        );
}

void AnnotationDialog::CompletableLineEdit::handleSpecialKeysInSearch( QKeyEvent* ev )
{
    int cursorPos = cursorPosition();

    QString txt = text().left(cursorPos) + ev->text() + text().mid( cursorPos );
    setText( txt );
    setCursorPosition( cursorPos + ev->text().length() );
    deselect();

    // Select the item in the listbox - not perfect but acceptable for now.
    int start = txt.findRev( QRegExp(QString::fromLatin1("[!&|]")), cursorPosition() -2 ) +1;
    QString input = txt.mid( start, cursorPosition()-start-1 );

    if ( !input.isEmpty() ) {
        QListViewItem* item = findItemInListView( input );
        if ( item )
            item->setSelected( true );
    }
}

void AnnotationDialog::CompletableLineEdit::showOnlyItemsMatching( const QString& text )
{
    for ( QListViewItemIterator itemIt( _listbox ); *itemIt; ++itemIt )
        (*itemIt)->setVisible( itemMatchesText( *itemIt, text ) );
}

void AnnotationDialog::CompletableLineEdit::selectPrevNextMatch( bool next )
{
    int itemStart = text().findRev( QRegExp(QString::fromLatin1("[!&|]")) ) +1;
    QString input = text().mid( itemStart );

    QListViewItem* item = _listbox->findItem( input, 0 );
    if ( !item )
        return;

    if ( next )
        item = item->itemBelow();
    else
        item = item->itemAbove();

    if ( item )
        selectItemAndUpdateLineEdit( item, itemStart, text().left( selectionStart() ) );
}

void AnnotationDialog::CompletableLineEdit::selectItemAndUpdateLineEdit( QListViewItem* item,
                                                                         const int itemStart, const QString& inputText )
{
    _listbox->setCurrentItem( item );
    _listbox->ensureItemVisible( item );

    QString txt = text().left(itemStart) + item->text(0) + text().mid( cursorPosition() );

    setText( txt );
    setSelection( itemStart + inputText.length(), item->text(0).length() - inputText.length() );
}


