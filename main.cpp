#include "thumbnailview.h"
#include "options.h"
#include <qdir.h>
#include "mainview.h"
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

static const KCmdLineOptions options[] =
{
	{ "c ", I18N_NOOP("Config file"), 0 },
	{ 0, 0, 0}
};

int main( int argc, char** argv ) {
    KAboutData aboutData( "kpalbum", I18N_NOOP("K Photo Album"), "0.01",
                          I18N_NOOP("Virtual Photo Album for KDE"), KAboutData::License_GPL );
    aboutData.addAuthor( "Jesper K. Pedersen", "Development", "blackie@kde.org" );

    KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if ( args->isSet( "c" ) )
        Options::setConfFile( args->getOption( "c" ) );

    MainView* view = new MainView( 0,  "view" );
    view->resize(800, 600);
    view->show();

    QObject::connect( qApp, SIGNAL( lastWindowClosed() ), qApp, SLOT( quit() ) );
    return app.exec();
}
