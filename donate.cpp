#include "donate.h"
#include <qlayout.h>
#include <qlabel.h>
#include <krun.h>
#include <kurl.h>
#include <klocale.h>
#include <kstandarddirs.h>

Donate::Donate( QWidget* parent, const char* name )
    :KDialogBase( Plain, i18n("Donate money"), Close | User1, Close, parent, name )
{
    QWidget* top = plainPage();
    QHBoxLayout* layout = new QHBoxLayout( top, 10 );

    QLabel* image = new QLabel( top, "image" );
    image->setMinimumSize( QSize( 273, 204 ) );
    image->setMaximumSize( QSize( 273, 204 ) );
    image->setPixmap( locate("data", QString::fromLatin1("kimdaba/pics/splash.png") ) );
    layout->addWidget( image );

    QString txt = i18n("<qt><p><center><b>Donate Money to KimDaBa development</b></center></p>"

                       "<p>If you like KimDaBa, and would like it to evolve, then please "
                       "consider donating money to the KimDaBa developer.</p>"

                       "<p>Donating money will have the following effects:"
                       "<ul><li>The KimDaBa developer will be motivated "
                       "from your acknowledgment of his work."
                       "<li>The KimDaBa developer will get a chance to take "
                       "some time of from his daily work to be able to complete "
                       "larger features or reorganizations (In January 2004, he took for example, "
                       "a week of from work to focus on optimizations, EXIF handling, and "
                       "making KimDaBa independent of file reorganizations, this work resulted version 1.1)</ul></p>"

                       "Money may be donated by clicking on the Donate button below, which will take you "
                       "to paypal. Any donation is highly appreciated.</p></qt>");
    QLabel* label = new QLabel( txt, top );
    layout->addWidget( label );
    setButtonText( User1, i18n("Donate") );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotDonate() ) );
}

void Donate::slotDonate()
{
    KRun::runURL(KURL(QString::fromLatin1("https://www.paypal.com/xclick/business=blackie%40blackie.dk&item_name=KimDaBa")),
                 QString::fromLatin1( "text/html" ) );
}
