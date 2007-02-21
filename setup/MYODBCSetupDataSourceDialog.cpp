/* Copyright (C) 2000-2005 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   There are special exceptions to the terms and conditions of the GPL as it
   is applied to this software. View the full text of the exception in file
   EXCEPTIONS in the directory of this software distribution.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "MYODBCSetupDataSourceDialog.h"

#include "MySQL-16.xpm"
#include "TopImage.xpm"

#ifdef HAVE_CONFIG_H
#include "../driver/myconf.h" 
#define SETUP_VERSION VERSION
#endif

// 64bit sparc fails to get this so set it manually
#ifndef SETUP_VERSION
#define SETUP_VERSION "v3"
#endif

MYODBCSetupDataSourceDialog::MYODBCSetupDataSourceDialog( QWidget *pwidgetParent, MYODBCUTIL_DATASOURCE *pDataSource )
    : QDialog( pwidgetParent )
{
    this->hDBC              = NULL;
    this->pDataSource       = pDataSource;

    doInit();
}

MYODBCSetupDataSourceDialog::MYODBCSetupDataSourceDialog( QWidget *pwidgetParent, SQLHDBC hDBC, MYODBCUTIL_DATASOURCE *pDataSource )
    : QDialog( pwidgetParent )
{
    this->hDBC              = hDBC;
    this->pDataSource       = pDataSource;

    doInit();
}

MYODBCSetupDataSourceDialog::~MYODBCSetupDataSourceDialog()
{
    // Are we getting deleted - is our destructor getting executed?
    // printf( "[PAH][%s][%d]\n", __FILE__, __LINE__ );
}

// toggle display of diagnostic info
void MYODBCSetupDataSourceDialog::slotDiagnostics()
{
}

// invoke browser with online help
void MYODBCSetupDataSourceDialog::slotHelp()
{
#if QT_VERSION >= 0x040000
    // Qt v4
    QProcess  * pprocess    = new QProcess( this );
    QString     stringURL   = "http://dev.mysql.com/doc/mysql/en/ODBC_Connector.html";

#if defined( Q_WS_WIN )
    if ( pprocess->startDetached( "c:\\program files\\internet explorer\\IEXPLORE.EXE", QStringList() << stringURL ) )
        return;
#elif defined( Q_WS_MACX )
    if ( pprocess->startDetached( "open", QStringList() << stringURL ) )
        return;
#else
    // UNIX folks...
    if ( pprocess->startDetached( "netscape", QStringList() << stringURL ) )
        return;

    // KDE folks...
    delete pprocess;
    pprocess = new QProcess( this );
    if ( pprocess->startDetached( "konqueror", QStringList() << stringURL ) )
        return;

    // GNOME folks...
    delete pprocess;
    pprocess = new QProcess( this );
    if ( !pprocess->startDetached( "htmlview", QStringList() << stringURL ) )
        return;
#endif

#else
    // Qt v3
    QProcess  * pprocess    = new QProcess( this );
    QString     stringURL   = "http://dev.mysql.com/doc/mysql/en/ODBC_Connector.html";

#if defined( Q_WS_WIN )
    pprocess->addArgument( "c:\\program files\\internet explorer\\IEXPLORE.EXE" );
    pprocess->addArgument( stringURL );
    if ( pprocess->start() )
        return;
#elif defined( Q_WS_MACX )
    pprocess->addArgument( "open" );
    pprocess->addArgument( stringURL );
    if ( pprocess->start() )
        return;
#else
    // UNIX folks...
    pprocess->addArgument( "netscape" );
    pprocess->addArgument( stringURL );
    if ( pprocess->start() )
        return;

    // KDE folks...
    pprocess->clearArguments();
    pprocess->addArgument( "konqueror" );
    pprocess->addArgument( stringURL );
    if ( pprocess->start() )
        return;

    // GNOME folks...
    pprocess->clearArguments();
    pprocess->addArgument( "htmlview" );
    pprocess->addArgument( stringURL );
    if ( pprocess->start() )
        return;
#endif

#endif

    QMessageBox::warning( this, "MyODBC Help", "Failed to execute a browser.\n\nPlease visit\n" + stringURL, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
    delete pprocess;
}

// try to connect/disconnect
void MYODBCSetupDataSourceDialog::slotTest()
{
    if ( pDataSource->nMode == MYODBCUTIL_DATASOURCE_MODE_DRIVER_CONNECT )
        doTestUsingDriver();
    else
        doTestUsingDriverManager();
}

// save dsn info
void MYODBCSetupDataSourceDialog::slotOk()
{
    switch ( pDataSource->nMode )
    {
        case MYODBCUTIL_DATASOURCE_MODE_DSN_ADD:
            if ( ptab1->getDataSourceName().isEmpty() )
            {
                ptab1->plineeditDataSourceName->setFocus();
                QMessageBox::warning( this, "MyODBC", tr("Missing Data Source Name"), tr("&Ok"), QString::null, QString::null, 0, 1 );
                return;
            }
            if ( ptab1->getUser().isEmpty() )
            {
                ptab1->plineeditUser->setFocus();
                QMessageBox::warning( this, "MyODBC", tr("Missing User ID"), tr("&Ok"), QString::null, QString::null, 0, 1 );
                return;
            }

            /*!
                ODBC RULE
        
                If the data source name matches an existing data source name and hwndParent is null, 
                ConfigDSN overwrites the existing name. If it matches an existing name and hwndParent 
                is not null, ConfigDSN prompts the user to overwrite the existing name.        
            */
            if ( MYODBCUtilDSNExists( (char *)(ptab1->getDataSourceName().data()) ) )
            {
                int n = QMessageBox::warning( this, "MyODBC", tr("Data Source Name (%1) exists. It will be replaced?") .arg( ptab1->getDataSourceName() ), tr("&Ok"), tr("&Cancel"), QString::null, 0, 1 );
                if ( n )
                    return;
            }
            break;

        case MYODBCUTIL_DATASOURCE_MODE_DSN_EDIT:
            if ( ptab1->getDataSourceName().isEmpty() )
            {
#if QT_VERSION >= 0x040000
                ptabwidget->setCurrentWidget( 0 );
#else
                ptabwidget->setCurrentPage( 0 );
#endif
                ptab1->plineeditDataSourceName->setFocus();
                QMessageBox::warning( this, "MyODBC", tr("Missing Data Source Name"), tr("&Ok"), QString::null, QString::null, 0, 1 );
                return;
            }
            break;

        case MYODBCUTIL_DATASOURCE_MODE_DSN_VIEW:
            break;

        case MYODBCUTIL_DATASOURCE_MODE_DRIVER_CONNECT:
            /*
                MYODBC RULE

                A UID is really the only thing we MUST have for a driver connect.
            */                
            if ( ptab1->getUser().isEmpty() )
            {
#if QT_VERSION >= 0x040000
                ptabwidget->setCurrentWidget( 0 );
#else
                ptabwidget->setCurrentPage( 0 );
#endif
                ptab1->plineeditUser->setFocus();
                QMessageBox::warning( this, "MyODBC", tr("Missing User ID"), tr("&Ok"), QString::null, QString::null, 0, 1 );
                return;
            }
            break;

        default:
            break;
    }

    if ( pDataSource->nMode != MYODBCUTIL_DATASOURCE_MODE_DSN_VIEW )
    {
        // update MYODBCUTIL_DATASOURCE here
        MYODBCUtilClearDataSource( pDataSource );

#if QT_VERSION >= 0x040000
        if ( !ptab1->getDataSourceName().isEmpty() )
            pDataSource->pszDSN = _global_strdup( ptab1->getDataSourceName().toAscii() );
        if ( !ptab1->getDescription().isEmpty() )
            pDataSource->pszDESCRIPTION = _global_strdup( ptab1->getDescription().toAscii() );

        if ( !ptab1->getServer().isEmpty() )
            pDataSource->pszSERVER = _global_strdup( ptab1->getServer().toAscii() );
        if ( !ptab1->getUser().isEmpty() )
            pDataSource->pszUSER = _global_strdup( ptab1->getUser().toAscii() );
        if ( !ptab1->getPassword().isEmpty() )
            pDataSource->pszPASSWORD = _global_strdup( ptab1->getPassword().toAscii() );
        if ( !ptab1->getDatabase().isEmpty() )
            pDataSource->pszDATABASE = _global_strdup( ptab1->getDatabase().toAscii() );
        if ( !ptab2->getPort().isEmpty() )
            pDataSource->pszPORT = _global_strdup( ptab2->getPort().toAscii() );
        if ( !ptab2->getSocket().isEmpty() )
            pDataSource->pszSOCKET = _global_strdup( ptab2->getSocket().toAscii() );
        if ( !ptab2->getInitialStatement().isEmpty() )
            pDataSource->pszSTMT = _global_strdup( ptab2->getInitialStatement().toAscii() );
#else
        if ( !ptab1->getDataSourceName().isEmpty() )
            pDataSource->pszDSN = _global_strdup( ptab1->getDataSourceName().ascii() );
        if ( !ptab1->getDescription().isEmpty() )
            pDataSource->pszDESCRIPTION = _global_strdup( ptab1->getDescription().ascii() );

        if ( !ptab1->getServer().isEmpty() )
            pDataSource->pszSERVER = _global_strdup( ptab1->getServer().ascii() );
        if ( !ptab1->getUser().isEmpty() )
            pDataSource->pszUSER = _global_strdup( ptab1->getUser().ascii() );
        if ( !ptab1->getPassword().isEmpty() )
            pDataSource->pszPASSWORD = _global_strdup( ptab1->getPassword().ascii() );
        if ( !ptab1->getDatabase().isEmpty() )
            pDataSource->pszDATABASE = _global_strdup( ptab1->getDatabase().ascii() );
        if ( !ptab2->getPort().isEmpty() )
            pDataSource->pszPORT = _global_strdup( ptab2->getPort().ascii() );
        if ( !ptab2->getSocket().isEmpty() )
            pDataSource->pszSOCKET = _global_strdup( ptab2->getSocket().ascii() );
        if ( !ptab2->getInitialStatement().isEmpty() )
            pDataSource->pszSTMT = _global_strdup( ptab2->getInitialStatement().ascii() );
#endif

        unsigned int nFlags = ptab3->getFlags();

        if ( nFlags > 0 )
        {
            pDataSource->pszOPTION = (char*)malloc( 50 );
            sprintf( pDataSource->pszOPTION, "%d", nFlags );
        }
    }

    // exit 
    done( QDialog::Accepted );
}

void MYODBCSetupDataSourceDialog::doInit()
{
#if QT_VERSION >= 0x040000
    setWindowIcon( QPixmap( MySQL_16_xpm ) );
#else
    setIcon( QPixmap( MySQL_16_xpm ) );
#endif

#if QT_VERSION >= 0x040000
    playoutTop = new QVBoxLayout;
    setLayout( playoutTop );
#else
    playoutTop = new QVBoxLayout( this );
#endif
    playoutTop->setMargin( 0 );
    playoutTop->setSpacing( 0 );

    playoutLabels = new QHBoxLayout;
    playoutLabels->setMargin( 0 );
    playoutLabels->setSpacing( 0 );
    playoutTop->addLayout( playoutLabels );

    // labels (text/image)
    plabelText = new QLabel( "", this );
    plabelText->setPalette( Qt::white );
    plabelText->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    playoutLabels->addWidget( plabelText );
    playoutLabels->setStretchFactor( plabelText, 10 );

    plabelImage = new QLabel( this );
    plabelImage->setPixmap( TopImage_xpm );
    plabelImage->setMinimumSize( 500, 63 );
    plabelImage->setMaximumSize( 500, 63 );
    playoutLabels->addWidget( plabelImage );
#if QT_VERSION >= 0x040000
    plabelImage->setToolTip( "Brought to you by the database elves." );
#else
    QToolTip::add( plabelImage, "Brought to you by the database elves." );
#endif

    // contrasting underline below header
    QLabel *plabelSpacer = new QLabel( "", this );
    plabelSpacer->setFixedHeight( 2 );
    plabelSpacer->setPalette( QColor( "#000060608080" ) );
    playoutTop->addWidget( plabelSpacer );

    // layout for body below header
    playoutTop2 = new QVBoxLayout;
    playoutTop2->setMargin( 5 );
    playoutTop2->setSpacing( 5 );
    playoutTop->addLayout( playoutTop2 );

    // splitter
    psplitter = new QSplitter( this );
    playoutTop2->addWidget( psplitter );

    // tab widget in the middle
    ptabwidget = new QTabWidget( psplitter );

    // text browser (assist) to the right)
    ptextbrowserAssist = new MYODBCSetupAssistText( psplitter );

    // tabs
    ptab1 = new MYODBCSetupDataSourceTab1( ptabwidget );
    ptabwidget->addTab( ptab1, "Login" ); 
    ptab2 = new MYODBCSetupDataSourceTab2( ptabwidget );
    ptabwidget->addTab( ptab2, "Connect Options" ); 
    ptab3 = new MYODBCSetupDataSourceTab3( ptabwidget );
    ptabwidget->addTab( ptab3, "Advanced" ); 

    // buttons near bottom
    playoutButtons = new QHBoxLayout;
    playoutTop2->addLayout( playoutButtons );
    playoutButtons->addStretch( 10 );
    ppushbuttonTest = new QPushButton( "&Test", this );
    playoutButtons->addWidget( ppushbuttonTest );
    ppushbuttonDiagnostics = new QPushButton( "&Diagnostics >>", this );
// connecting to the database causes core on sparc for some reason (ld issue?)
#ifdef __sparc
    ppushbuttonTest->hide();
    ppushbuttonDiagnostics->hide();
#endif

#if QT_VERSION >= 0x040000
    ppushbuttonDiagnostics->setCheckable( TRUE );
#else
    ppushbuttonDiagnostics->setToggleButton( TRUE );
#endif
    playoutButtons->addWidget( ppushbuttonDiagnostics );
    playoutButtons->addStretch( 4 );
    ppushbuttonOk = new QPushButton( "&Ok", this );
    playoutButtons->addWidget( ppushbuttonOk );
    ppushbuttonCancel = new QPushButton( "&Cancel", this );
    playoutButtons->addWidget( ppushbuttonCancel );
    ppushbuttonHelp = new QPushButton( "&Help", this );
    playoutButtons->addWidget( ppushbuttonHelp );

    // lets hide this until we have completed the feature
    // ppushbuttonDiagnostics->hide();

    // diagnostics along bottom
    ptexteditDiagnostics = new QTextEdit( this );
    ptexteditDiagnostics->hide();
    playoutTop2->addWidget( ptexteditDiagnostics );

    // set values
    ptab1->setDatabase( pDataSource->pszDATABASE );
    ptab1->setDescription( pDataSource->pszDESCRIPTION );
    ptab1->setDataSourceName( pDataSource->pszDSN );
    ptab1->setPassword( pDataSource->pszPASSWORD );
    ptab2->setPort( pDataSource->pszPORT );
    ptab1->setServer( pDataSource->pszSERVER );
    ptab2->setSocket( pDataSource->pszSOCKET );
    ptab2->setInitialStatement( pDataSource->pszSTMT );
    ptab1->setUser( pDataSource->pszUSER );

    if ( pDataSource->pszOPTION )
    {
        ulong nOptions = atol( pDataSource->pszOPTION );
        ptab3->ptab3a->pcheckboxDontOptimizeColumnWidth->setChecked( nOptions & (1 << 0) ? TRUE : FALSE );
        ptab3->ptab3a->pcheckboxReturnMatchingRows->setChecked( nOptions & (1 << 1) ? TRUE : FALSE );
        ptab3->ptab3d->pcheckboxTraceDriverCalls->setChecked( nOptions & (1 << 2) ? TRUE : FALSE );
        ptab3->ptab3a->pcheckboxAllowBigResults->setChecked( nOptions & (1 << 3) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxDontPromptOnConnect->setChecked( nOptions & (1 << 4) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxEnableDynamicCursor->setChecked( nOptions & (1 << 5) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxIgnorePoundInTable->setChecked( nOptions & (1 << 6) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxUseManagerCursors->setChecked( nOptions & (1 << 7) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxDontUseSetLocale->setChecked( nOptions & (1 << 8) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxPadCharToFullLen->setChecked( nOptions & (1 << 9) ? TRUE : FALSE );
        ptab3->ptab3c->pcheckboxReturnTableNamesSQLDescribeCol->setChecked( nOptions & (1 << 10) ? TRUE : FALSE );
        ptab3->ptab3a->pcheckboxUseCompressedProtocol->setChecked( nOptions & (1 << 11) ? TRUE : FALSE );
        ptab3->ptab3c->pcheckboxIgnoreSpaceAfterFunctionNames->setChecked( nOptions & (1 << 12) ? TRUE : FALSE ); 
        ptab3->ptab3c->pcheckboxForceUseOfNamedPipes->setChecked( nOptions & (1 << 13) ? TRUE : FALSE );          
        ptab3->ptab3a->pcheckboxChangeBIGINTColumnsToInt->setChecked( nOptions & (1 << 14) ? TRUE : FALSE );
        ptab3->ptab3c->pcheckboxNoCatalog->setChecked( nOptions & (1 << 15) ? TRUE : FALSE );
        ptab3->ptab3c->pcheckboxReadOptionsFromMyCnf->setChecked( nOptions & (1 << 16) ? TRUE : FALSE );          
        ptab3->ptab3a->pcheckboxSafe->setChecked( nOptions & (1 << 17) ? TRUE : FALSE );
        ptab3->ptab3c->pcheckboxDisableTransactions->setChecked( nOptions & (1 << 18) ? TRUE : FALSE );           
        ptab3->ptab3d->pcheckboxSaveQueries->setChecked( nOptions & (1 << 19) ? TRUE : FALSE );
        ptab3->ptab3b->pcheckboxDontCacheResults->setChecked( nOptions & (1 << 20) ? TRUE : FALSE );
        ptab3->ptab3c->pcheckboxForceUseOfForwardOnlyCursors->setChecked( nOptions & (1 << 21) ? TRUE : FALSE );  
        ptab3->ptab3a->pcheckboxEnableReconnect->setChecked( nOptions & (1 << 22) ? TRUE : FALSE );
        ptab3->ptab3a->pcheckboxAutoIncrementIsNull->setChecked( nOptions & (1 << 23) ? TRUE : FALSE );
    }

    connect( ppushbuttonTest, SIGNAL(clicked()), SLOT(slotTest()) );
    connect( ppushbuttonDiagnostics, SIGNAL(toggled(bool)), SLOT(slotToggleGuru(bool)) );
    connect( ppushbuttonHelp, SIGNAL(clicked()), SLOT(slotHelp()) );
    connect( ppushbuttonOk, SIGNAL(clicked()), SLOT(slotOk()) );
    connect( ppushbuttonCancel, SIGNAL(clicked()), SLOT(reject()) );

#ifndef __sparc
    connect( ptab1, SIGNAL(signalRequestDatabaseNames()), SLOT(slotLoadDatabaseNames()) );
#endif

    connect( ptab1->plineeditDataSourceName, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab1->plineeditDescription, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab1->plineeditServer, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab1->plineeditUser, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab1->plineeditPassword, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab1->pcomboboxDatabase, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab2->plineeditPort, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab2->plineeditSocket, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab2->plineeditInitialStatement, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxDontOptimizeColumnWidth, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxReturnMatchingRows, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxAllowBigResults, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxUseCompressedProtocol, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxChangeBIGINTColumnsToInt, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxSafe, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxEnableReconnect, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3a->pcheckboxAutoIncrementIsNull, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxDontPromptOnConnect, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxEnableDynamicCursor, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxIgnorePoundInTable, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxUseManagerCursors, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxDontUseSetLocale, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxPadCharToFullLen, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3b->pcheckboxDontCacheResults, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxReturnTableNamesSQLDescribeCol, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxIgnoreSpaceAfterFunctionNames, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxForceUseOfNamedPipes, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxNoCatalog, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxReadOptionsFromMyCnf, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxDisableTransactions, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3c->pcheckboxForceUseOfForwardOnlyCursors, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3d->pcheckboxTraceDriverCalls, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );
    connect( ptab3->ptab3d->pcheckboxSaveQueries, SIGNAL(signalAssistText(const QString&)), ptextbrowserAssist, SLOT(setHtml(const QString&)) );

    doApplyMode();
}

void MYODBCSetupDataSourceDialog::doApplyMode()
{
    switch ( pDataSource->nMode )
    {
        case MYODBCUTIL_DATASOURCE_MODE_DSN_EDIT:
#if QT_VERSION >= 0x040000
            setWindowTitle( "Connector/ODBC " SETUP_VERSION " - Configure Data Source Name" );
#else
            setCaption( "Connector/ODBC " SETUP_VERSION " - Configure Data Source Name" );
#endif
            ptextbrowserAssist->setDefaultHtml( tr("<B>Connector/ODBC Configuration</B><HR><P>This dialog is used to edit a Data Source Name (DSN).") );
            break;
        case MYODBCUTIL_DATASOURCE_MODE_DSN_ADD:
#if QT_VERSION >= 0x040000
            setWindowTitle( "Connector/ODBC " SETUP_VERSION " - Add Data Source Name" );
#else
            setCaption( "Connector/ODBC " SETUP_VERSION " - Add Data Source Name" );
#endif
            ptextbrowserAssist->setDefaultHtml( tr("<B>Connector/ODBC Configuration</B><HR><P>This dialog is used to add a Data Source Name (DSN).") );
            break;
        case MYODBCUTIL_DATASOURCE_MODE_DSN_VIEW:
#if QT_VERSION >= 0x040000
            setWindowTitle( "Connector/ODBC " SETUP_VERSION " - View Data Source Name" );
#else
            setCaption( "Connector/ODBC " SETUP_VERSION " - View Data Source Name" );
#endif
            ptextbrowserAssist->setDefaultHtml( tr("<B>Connector/ODBC Configuration</B><HR><P>This dialog is used to view a Data Source Name (DSN).") );
            break;
        case MYODBCUTIL_DATASOURCE_MODE_DRIVER_CONNECT:
#if QT_VERSION >= 0x040000
            setWindowTitle( "Connector/ODBC " SETUP_VERSION " - Driver Connect" );
#else
            setCaption( "Connector/ODBC " SETUP_VERSION " - Driver Connect" );
#endif
            ptextbrowserAssist->setDefaultHtml( tr("<B>Connector/ODBC Configuration</B><HR><P>This dialog is used to connect to a Data Source Name (DSN).") );
            ppushbuttonTest->hide();
            ppushbuttonDiagnostics->hide();
            switch ( pDataSource->nPrompt )
            {
                case MYODBCUTIL_DATASOURCE_PROMPT_COMPLETE:
                    /*
                        ODBC RULE

                        We assume caller has already tried to connect and failed. So we 
                        do same thing as PROMPT.
                    */    
                case MYODBCUTIL_DATASOURCE_PROMPT_PROMPT:
                    /* 
                        MYODBC RULE
                        
                        Hide DSN fields if we are doing a DSN-less connect. 
                    */
                    if ( pDataSource->nConnect != MYODBCUTIL_DATASOURCE_CONNECT_DSN )
                    {
                        ptab1->plabelDataSourceName->hide();
                        ptab1->plineeditDataSourceName->hide();

                        ptab1->plabelDescription->hide();
                        ptab1->plineeditDescription->hide();

                        ptab1->plineeditUser->setFocus();
                    }
                    break;
                case MYODBCUTIL_DATASOURCE_PROMPT_REQUIRED:
                    /*
                        ODBC RULE

                        We assume caller has already tried to connect and failed. So we 
                        prompt - but only with the required fields.
                    */    
                    if ( pDataSource->nConnect == MYODBCUTIL_DATASOURCE_CONNECT_DSN )
                    {
                        /*
                            MYODBC RULE

                            Disable everything except UID and PWD. The rest must be configured
                            in the DSN.
                        */    
                        ptab1->plineeditDataSourceName->setDisabled( true );
                        ptab1->plineeditDescription->setDisabled( true );
                        ptab1->plineeditServer->setDisabled( true );
                        ptab1->pcomboboxDatabase->setDisabled( true );
                        ptab2->setDisabled( true );
                        ptab3->setDisabled( true );
                    }
                    else
                    {
                        /*
                            MYODBC RULE

                            Hide DSN specific stuff. Disable server and database only
                            if provided. UID and PWD is enabled and remaining disabled. 
                        */                            
                        ptab1->plabelDataSourceName->hide();
                        ptab1->plineeditDataSourceName->hide();

                        ptab1->plabelDescription->hide();
                        ptab1->plineeditDescription->hide();

                        if ( pDataSource->pszSERVER )
                            ptab1->plineeditServer->setDisabled( true );
                        if ( pDataSource->pszDATABASE )
                            ptab1->pcomboboxDatabase->setDisabled( true );
                        ptab2->setDisabled( true );
                        ptab3->setDisabled( true );
                    }

                    ptab1->plineeditUser->setFocus();
                    break;
                case MYODBCUTIL_DATASOURCE_PROMPT_NOPROMPT:
                    /* this should not happen */
                    break;
            }
            break;
        default:
#if QT_VERSION >= 0x040000
            setWindowTitle( tr("Connector/ODBC - Unknown Mode") );
#else
            setCaption( tr("Connector/ODBC - Unknown Mode") );
#endif
    }
}

/*!
    \brief  Try connect/disconnect.

            Here we try the connect/disconnect by calling into
            the driver manager. This is a good test and is used when
            we are working on behalf of the ODBC administrator but
            we can not do a test this way when we are being called by
            the driver because we do not want to create another
            ODBC environment within the same app context and we can
            not use an existing DBC (from SQLDriverConnect) because
            it would be the drivers DBC - not the DM's DBC.

    \sa     doTestUsingMySQL            
*/
BOOL MYODBCSetupDataSourceDialog::doTestUsingDriverManager()
{
    SQLHENV     hEnv        = SQL_NULL_HENV;
    SQLHDBC     hDbc        = SQL_NULL_HDBC;
    SQLRETURN   nReturn;
    QString     stringConnectIn;

    /* build connect string in */
#ifdef Q_WS_MACX
    /*!
        \note OSX

              Yet another place where iODBC fails to act like the ODBC
              specification. SQLDriverConnect() should accept the 
              driver *description* and should allow for the curly
              braces but fails on both accounts deciding instead to
              interpret the whole value as a driver file name.

              Its possible to do the xref here, for SQLDriverConnect,
              but due to time constraints the file is hard-coded.
    */ 
    stringConnectIn  = "Driver=/usr/lib/libmyodbc3.dylib";
#else
    stringConnectIn  = "DRIVER=" + QString( pDataSource->pszDRIVER );
#endif
    stringConnectIn += ";UID=" + ptab1->getUser();
    stringConnectIn += ";PWD=" + ptab1->getPassword();
    stringConnectIn += ";SERVER=" + ptab1->getServer();
    if ( !ptab1->getDatabase().isEmpty() )
        stringConnectIn += ";DATABASE=" + ptab1->getDatabase();
    if ( !ptab2->getPort().isEmpty() )
        stringConnectIn += ";PORT=" + ptab2->getPort();
    if ( !ptab2->getSocket().isEmpty() )
        stringConnectIn += ";SOCKET=" + ptab2->getSocket();
//    if ( !ptab1->getOptions().isEmpty() )
//        stringConnectIn += ";OPTION=" + ptab1->getOptions();
    if ( !ptab2->getInitialStatement().isEmpty() )
        stringConnectIn += ";STMT=" + ptab2->getInitialStatement();

    nReturn = SQLAllocHandle( SQL_HANDLE_ENV, NULL, &hEnv );
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_ENV, NULL );
    if ( !SQL_SUCCEEDED(nReturn) )
        return FALSE;

    nReturn = SQLSetEnvAttr( hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0 );
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_ENV, NULL );
    if ( !SQL_SUCCEEDED(nReturn) )
        goto slotTestExit1;

    nReturn = SQLAllocHandle( SQL_HANDLE_DBC, hEnv, &hDbc );
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_ENV, hEnv );
    if ( !SQL_SUCCEEDED(nReturn) )
        goto slotTestExit1;

#if QT_VERSION >= 0x040000
    nReturn = SQLDriverConnect( hDbc, NULL, (SQLCHAR*)stringConnectIn.toAscii().data(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
#else
    nReturn = SQLDriverConnect( hDbc, NULL, (SQLCHAR*)stringConnectIn.latin1(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
#endif
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_DBC, hDbc );
    if ( !SQL_SUCCEEDED(nReturn) )
        goto slotTestExit2;

    QMessageBox::information( this, "Connector/ODBC", "Success; connection was made!\n", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );

    nReturn = SQLDisconnect( hDbc );

slotTestExit2:
    nReturn = SQLFreeHandle( SQL_HANDLE_DBC, hDbc );
slotTestExit1:
    nReturn = SQLFreeHandle( SQL_HANDLE_ENV, hEnv );

    return SQL_SUCCEEDED(nReturn);
}

/*!
    \brief  Try connect/disconnect.

            Here we try to use the driver directly as this is the dbc we
            have at hand.

    \note   The creation of this method was driven by problems while 
            coding support for SQLDriverConnect() prompting.
            
    \sa     doTestUsingDriverManager
*/
BOOL MYODBCSetupDataSourceDialog::doTestUsingDriver()
{
    QMessageBox::warning( this, "Connector/ODBC", "Testing not implemented for SQLDriverConnect()", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
    return TRUE;
}

BOOL MYODBCSetupDataSourceDialog::doLoadDatabaseNamesUsingDriverManager()
{
    SQLHENV     hEnv        = SQL_NULL_HENV;
    SQLHDBC     hDbc        = hDBC;
    SQLHSTMT    hStmt;
    SQLRETURN   nReturn;
    QString     stringConnectIn;
    QStringList stringlistDatabases;
    SQLCHAR     szCatalog[MYODBC_DB_NAME_MAX]; 
    SQLLEN      nCatalog;

    stringlistDatabases += " ";

    /* build connect string in */
#ifdef Q_WS_MACX
    /*!
        \note OSX

              Yet another place where iODBC fails to act like the ODBC
              specification. SQLDriverConnect() should accept the 
              driver *description* and should allow for the curly
              braces but fails on both accounts deciding instead to
              interpret the whole value as a driver file name.

              Its possible to do the xref here, for SQLDriverConnect,
              but due to time constraints the file is hard-coded.
    */ 
    stringConnectIn  = "Driver=/usr/lib/libmyodbc3.dylib";
#else
    stringConnectIn  = "DRIVER=" + QString( pDataSource->pszDRIVER );
#endif
    stringConnectIn += ";UID=" + ptab1->getUser();
    stringConnectIn += ";PWD=" + ptab1->getPassword();
    stringConnectIn += ";SERVER=" + ptab1->getServer();
    if ( !ptab2->getPort().isEmpty() )
        stringConnectIn += ";PORT=" + ptab2->getPort();
    if ( !ptab2->getSocket().isEmpty() )
        stringConnectIn += ";SOCKET=" + ptab2->getSocket();
//    if ( !plineeditOptions->text().isEmpty() )
//        stringConnectIn += ";OPTION=" + plineeditOptions->text();

    if ( hDBC == SQL_NULL_HDBC )
    {
        nReturn = SQLAllocHandle( SQL_HANDLE_ENV, NULL, &hEnv );
        if ( nReturn != SQL_SUCCESS )
            slotShowDiagnostics( nReturn, SQL_HANDLE_ENV, NULL );
        if ( !SQL_SUCCEEDED(nReturn) )
            return FALSE;
    
        nReturn = SQLSetEnvAttr( hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0 );
        if ( nReturn != SQL_SUCCESS )
            slotShowDiagnostics( nReturn, SQL_HANDLE_ENV, NULL );
        if ( !SQL_SUCCEEDED(nReturn) )
            goto slotLoadDatabaseNamesExit1;
    
        nReturn = SQLAllocHandle( SQL_HANDLE_DBC, hEnv, &hDbc );
        if ( nReturn != SQL_SUCCESS )
            slotShowDiagnostics( nReturn, SQL_HANDLE_ENV, hEnv );
        if ( !SQL_SUCCEEDED(nReturn) )
            goto slotLoadDatabaseNamesExit1;
    }

#if QT_VERSION >= 0x040000
    nReturn = SQLDriverConnect( hDbc, NULL, (SQLCHAR*)stringConnectIn.toAscii().data(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
#else
    nReturn = SQLDriverConnect( hDbc, NULL, (SQLCHAR*)stringConnectIn.latin1(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
#endif
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_DBC, hDbc );
    if ( !SQL_SUCCEEDED(nReturn) )
        goto slotLoadDatabaseNamesExit2;

    nReturn = SQLAllocHandle( SQL_HANDLE_STMT, hDbc, &hStmt );
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_DBC, hDbc );
    if ( !SQL_SUCCEEDED(nReturn) )
        goto slotLoadDatabaseNamesExit2;

    nReturn = SQLTables( hStmt, (SQLCHAR*)SQL_ALL_CATALOGS, SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"", 0, (SQLCHAR*)"", 0 );
    if ( nReturn != SQL_SUCCESS )
        slotShowDiagnostics( nReturn, SQL_HANDLE_STMT, hStmt );
    if ( !SQL_SUCCEEDED(nReturn) )
        goto slotLoadDatabaseNamesExit3;

    nReturn = SQLBindCol( hStmt, 1, SQL_C_CHAR, szCatalog, MYODBC_DB_NAME_MAX, &nCatalog );
    while ( TRUE )
    {
        nReturn = SQLFetch( hStmt );
        if ( nReturn == SQL_NO_DATA )
            break;
        else if ( nReturn != SQL_SUCCESS )
            slotShowDiagnostics( nReturn, SQL_HANDLE_STMT, hStmt );
        if ( SQL_SUCCEEDED(nReturn) )
            stringlistDatabases += (const char*)szCatalog;
        else
            break;
    }

slotLoadDatabaseNamesExit3:
    nReturn = SQLFreeHandle( SQL_HANDLE_STMT, hStmt );
slotLoadDatabaseNamesExit2:
    nReturn = SQLDisconnect( hDbc );
    if ( hDBC == SQL_NULL_HDBC )
        nReturn = SQLFreeHandle( SQL_HANDLE_DBC, hDbc );
slotLoadDatabaseNamesExit1:
    if ( hDBC == SQL_NULL_HDBC )
        nReturn = SQLFreeHandle( SQL_HANDLE_ENV, hEnv );

    ptab1->pcomboboxDatabase->clear();
#if QT_VERSION >= 0x040000
    ptab1->pcomboboxDatabase->addItems( stringlistDatabases );
#else
    ptab1->pcomboboxDatabase->insertStringList( stringlistDatabases );
#endif

    return TRUE;
}

BOOL MYODBCSetupDataSourceDialog::doLoadDatabaseNamesUsingDriver()
{
    QMessageBox::warning( this, "Connector/ODBC", "Loading database list not implemented for SQLDriverConnect()", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
    return TRUE;
}

void MYODBCSetupDataSourceDialog::slotShowDiagnostics( SQLRETURN nReturn, SQLSMALLINT nHandleType, SQLHANDLE h )
{
    BOOL        bDiagnostics = FALSE;
    SQLSMALLINT nRec = 1;
    SQLCHAR     szSQLState[6];
    SQLINTEGER  nNative;
    SQLCHAR     szMessage[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT nMessage;

    if ( h )
    {
        *szSQLState = '\0';
        *szMessage  = '\0';

        while ( SQL_SUCCEEDED( SQLGetDiagRec( nHandleType,
                                              h,
                                              nRec,
                                              szSQLState,
                                              &nNative,
                                              szMessage,
                                              SQL_MAX_MESSAGE_LENGTH,
                                              &nMessage ) ) )
        {
            szSQLState[5]               = '\0';
            szMessage[SQL_MAX_MESSAGE_LENGTH - 1]  = '\0';

#if QT_VERSION >= 0x040000
            ptexteditDiagnostics->setPlainText( ptexteditDiagnostics->toPlainText() + QString( "\n" ) + QString( (const char*)szMessage ) );
#else
            ptexteditDiagnostics->setText( ptexteditDiagnostics->text() + QString( "\n" ) + QString( (const char*)szMessage ) );
#endif
            bDiagnostics = TRUE;
            nRec++;

            *szSQLState = '\0';
            *szMessage  = '\0';
        }
    }

    switch ( nReturn )
    {
        case SQL_ERROR:
            QMessageBox::critical( this, "MYODBCConfig", "Request returned with SQL_ERROR.", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
            break;
        case SQL_SUCCESS_WITH_INFO:
            QMessageBox::warning( this, "MYODBCConfig", "Request return with SQL_SUCCESS_WITH_INFO.", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
            break;
        case SQL_INVALID_HANDLE:
            QMessageBox::critical( this, "MYODBCConfig", "Request returned with SQL_INVALID_HANDLE.", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
            break;
        default:
            QMessageBox::information( this, "MYODBCConfig", "Request did not return with SQL_SUCCESS.", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
    }
}

void MYODBCSetupDataSourceDialog::slotShowInstallerError()
{
    WORD      nRecord = 1;
    DWORD     nError;
    char      szError[SQL_MAX_MESSAGE_LENGTH];
    RETCODE   nReturn;

    nReturn = SQLInstallerError( nRecord, &nError, szError, SQL_MAX_MESSAGE_LENGTH - 1, 0 );
    if ( SQL_SUCCEEDED( nReturn ) )
        QMessageBox::critical( this, "MYODBCConfig", (char*)szError, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
    else
        QMessageBox::critical( this, "MYODBCConfig", "ODBC Installer error (unknown)", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
}

void MYODBCSetupDataSourceDialog::slotToggleGuru( bool b )
{
    if ( b )
    {
        ptexteditDiagnostics->show();
        ppushbuttonDiagnostics->setText( tr("&Diagnostics <<") );
    }
    else
    {
        ptexteditDiagnostics->hide();
        ppushbuttonDiagnostics->setText( tr("&Diagnostics >>") );
    }
}

void MYODBCSetupDataSourceDialog::slotLoadDatabaseNames()
{
    if ( hDBC )
        doLoadDatabaseNamesUsingDriver();
    else
        doLoadDatabaseNamesUsingDriverManager();
}

