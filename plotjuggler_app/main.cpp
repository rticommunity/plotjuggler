#include "mainwindow.h"
#include <iostream>
#include <QApplication>
#include <QSplashScreen>
#include <QThread>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QDir>

#include "PlotJuggler/transform_function.h"
#include "transforms/first_derivative.h"
#include "transforms/scale_transform.h"
#include "transforms/moving_average_filter.h"
#include "transforms/outlier_removal.h"
#include "transforms/integral_transform.h"

#include "nlohmann_parsers.h"
#include "new_release_dialog.h"

static QString VERSION_STRING = QString("%1.%2.%3").arg(PJ_MAJOR_VERSION).arg(PJ_MINOR_VERSION).arg(PJ_PATCH_VERSION);

inline int GetVersionNumber(QString str)
{
  QStringList online_version = str.split('.');
  if( online_version.size() != 3 )
  {
    return 0;
  }
  int major = online_version[0].toInt();
  int minor = online_version[1].toInt();
  int patch = online_version[2].toInt();
  return major * 10000 + minor * 100 + patch;
}

void OpenNewReleaseDialog(QNetworkReply* reply)
{
  if (reply->error())
  {
    return;
  }
  QString answer = reply->readAll();
  QJsonDocument document = QJsonDocument::fromJson(answer.toUtf8());
  QJsonObject data = document.object();
  QString url = data["html_url"].toString();
  QString name = data["name"].toString();
  QString tag_name = data["tag_name"].toString();
  QSettings settings;
  int online_number = GetVersionNumber(tag_name);
  QString dont_show = settings.value("NewRelease/dontShowThisVersion", VERSION_STRING).toString();
  int dontshow_number = GetVersionNumber(dont_show);
  int current_number = GetVersionNumber(VERSION_STRING);

  if (online_number > current_number && online_number > dontshow_number)
  {
    NewReleaseDialog* dialog = new NewReleaseDialog(nullptr, tag_name, name, url);
    dialog->show();
  }
}

QPixmap getFunnySplashscreen()
{
  QSettings settings;
  srand (time(nullptr));

  auto getNum = []() {
    const int last_image_num = 60;
    int n = rand() % (last_image_num + 2);
    if (n > last_image_num)
    {
      n = 0;
    }
    return n;
  };
  int n = getNum();
  int prev_n = settings.value("previousFunnySubtitle").toInt();
  while (n == prev_n)
  {
    n = getNum();
  }
  settings.setValue("previousFunnySubtitle", n);
  auto filename = QString("://resources/memes/meme_%1.jpg").arg(n, 2, 10, QChar('0'));
  return QPixmap(filename);
}



#define MERGE_ARGUMENTS_SIZE (32)
typedef char *MergedArguments[MERGE_ARGUMENTS_SIZE];

/*
 * Merges any command-line arguments provided in the definition of
 * PJ_DEFAULT_ARGS (e.g. using add_definitions in CMake with
 * the argumemts specified in the command-line.
 */
int merge_default_arguments(MergedArguments &new_argv, int argc, char* argv[])
{
  QStringList default_cmdline_args;
  
#ifdef PJ_DEFAULT_ARGS
  default_cmdline_args = QString(PJ_DEFAULT_ARGS).split(" ", QString::SkipEmptyParts);
#endif 

  int max_cmmand_line_args = MERGE_ARGUMENTS_SIZE - default_cmdline_args.size();
  if ( argc > max_cmmand_line_args )
  {
    qDebug() << "Maximum arguments exceeded. Limit is " << max_cmmand_line_args << "got " << argc; 
    return -1;
  }

  // preserve arg[0] => execuatle path
  QStringList merged_args(argv[0]);

  // Add the remain arguments, reoplacing escaped characters. 
  // Escaping needed because some chars cannot be entered easily in the -DPJ_DEFAULT_ARGS preprocessor directive
  //   _0x20_   -->   ' '   (space)
  //   _0x3b_   -->   ';'   (semicolon)
  for ( const auto& cmdline_arg : default_cmdline_args  )
  {
    // replace(const QString &before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    QString unscaped_arg(cmdline_arg);
    merged_args << unscaped_arg.replace("_0x20_", " ", Qt::CaseSensitive).replace("_0x3b_", ";", Qt::CaseSensitive);
  }

  // Then add the arguments entered to the command-lien with no replacements
  // If an argument is repeated it overrides the 'default' setting
  for (int i=1; i< argc; ++i ) {
    merged_args << argv[i];
  }

  int new_argc = merged_args.size();;
  for (int i=0; i< new_argc; ++i) {
    new_argv[i] = strdup(merged_args.at(i).toLocal8Bit().data());
  }

  return new_argc;
}


int main(int argc, char* argv[])
{
  MergedArguments new_argv;
  int new_argc = merge_default_arguments(new_argv, argc, argv);
  if ( new_argc < 0 ) 
  {
    return -1;
  }
  QApplication app(new_argc, new_argv);

  QCoreApplication::setOrganizationName("PlotJuggler");
  QCoreApplication::setApplicationName("PlotJuggler-3");
  QSettings::setDefaultFormat(QSettings::IniFormat);

  QSettings settings;

  if( !settings.isWritable() )
  {
    qDebug() << "ERROR: the file [" << settings.fileName() <<
                "] is not writable. This may happen when you run PlotJuggler with sudo. "
                "Change the permissions of the file (\"sudo chmod 666 <file_name>\"on linux)";
  }

  app.setApplicationVersion(VERSION_STRING);

  //---------------------------
  TransformFactory::registerTransform<FirstDerivative>();
  TransformFactory::registerTransform<ScaleTransform>();
  TransformFactory::registerTransform<MovingAverageFilter>();
  TransformFactory::registerTransform<OutlierRemovalFilter>();
  TransformFactory::registerTransform<IntegralTransform>();
  //---------------------------

  QCommandLineParser parser;
  parser.setApplicationDescription("PlotJuggler: the time series visualization tool that you deserve ");
  parser.addVersionOption();
  parser.addHelpOption();

  QCommandLineOption nosplash_option(QStringList() << "n"
                                                   << "nosplash",
                                     "Don't display the splashscreen");
  parser.addOption(nosplash_option);

  QCommandLineOption test_option(QStringList() << "t"
                                               << "test",
                                 "Generate test curves at startup");
  parser.addOption(test_option);

  QCommandLineOption loadfile_option(QStringList() << "d"
                                                   << "datafile",
                                     "Load a file containing data", "file_path");
  parser.addOption(loadfile_option);

  QCommandLineOption layout_option(QStringList() << "l"
                                                 << "layout",
                                   "Load a file containing the layout configuration", "file_path");
  parser.addOption(layout_option);

  QCommandLineOption publish_option(QStringList() << "p"
                                                  << "publish",
                                    "Automatically start publisher when loading the layout file");
  parser.addOption(publish_option);

  QCommandLineOption folder_option(QStringList() << "plugin_folders",
                                    "Add semicolon-separated list of folders where you should look for additional plugins.",
                                   "directory_paths");
  parser.addOption(folder_option);

  QCommandLineOption buffersize_option(QStringList() << "buffer_size",
                                       QCoreApplication::translate("main", "Change the maximum size of the streaming "
                                                                           "buffer (minimum: 10 default: 60)"),
                                       QCoreApplication::translate("main", "seconds"));
  parser.addOption(buffersize_option);

  QCommandLineOption nogl_option(QStringList() << "disable_opengl",
                                 "Disable OpenGL display before starting the application. "
                                 "You can enable it again in the 'Preferences' menu.");
  parser.addOption(nogl_option);



  QCommandLineOption enabled_plugins_option(QStringList() << "enabled_plugins",
                                     "Limit the loaded plugins to ones in the semicolon-separated list", "name_list");
  parser.addOption(enabled_plugins_option);

  QCommandLineOption disabled_plugins_option(QStringList() << "disabled_plugins",
                                     "Do not load any of the plugins in the semicolon separated list", "name_list");
  parser.addOption(disabled_plugins_option);

  QCommandLineOption selected_streamer_option(QStringList() << "selected_streamer",
                                    "Make the specified streaming plugin the first selection in the menu", "name");
  parser.addOption(selected_streamer_option); 

  QCommandLineOption subscribe_option(QStringList() << "s"
                                                    << "subscribe",
                                     "Automatically start the default streaming plugin", "bool_value", "false");
  parser.addOption(subscribe_option); 

  QCommandLineOption title_option(QStringList() << "title",
                                     "Use the specified text for the main window title", "text");
  parser.addOption(title_option);

  QCommandLineOption filesplash_option(QStringList() << "splash",
                                     "Load a file containing the splash screen", "file_path");
  parser.addOption(filesplash_option);

  QCommandLineOption about_title_option(QStringList() << "about_title",
                                     "Load a file containing the title section for the about dialog", "file_path");
  parser.addOption(about_title_option);

  QCommandLineOption about_body_option(QStringList() << "about_body",
                                     "Load a file containing the body section for the about dialogn", "file_path");
  parser.addOption(about_body_option);


  parser.process(*qApp);

  if (parser.isSet(publish_option) && !parser.isSet(layout_option))
  {
    std::cerr << "Option [ -p / --publish ] is invalid unless [ -l / --layout ] is used too." << std::endl;
    return -1;
  }

  if (parser.isSet(enabled_plugins_option) && parser.isSet(disabled_plugins_option))
  {
    std::cerr << "Option [ --enabled_plugins ] and [ --disabled_plugins ] can't be used together." << std::endl;
    return -1;
  }

  if (parser.isSet(nogl_option))
  {
    settings.setValue("Preferences::use_opengl", false);
  }

  QIcon app_icon("://resources/plotjuggler.svg");
  QApplication::setWindowIcon(app_icon);

  QNetworkAccessManager manager;
  QObject::connect(&manager, &QNetworkAccessManager::finished, OpenNewReleaseDialog);

  QNetworkRequest request;
  request.setUrl(QUrl("https://api.github.com/repos/facontidavide/PlotJuggler/releases/latest"));
  manager.get(request);

  /*
   * You, fearless code reviewer, decided to start a journey into my source code.
   * For your bravery, you deserve to know the truth.
   * The splashscreen is useless; not only it is useless, it will make your start-up
   * time slower by few seconds for absolutely no reason.
   * But what are two seconds compared with the time that PlotJuggler will save you?
   * The splashscreen is the connection between me and my users, the glue that keeps
   * together our invisible relationship.
   * Now, it is up to you to decide: you can block the splashscreen forever or not,
   * reject a message that brings a little of happiness into your day, spent analyzing data.
   * Please don't do it.
   */

  MainWindow *w;

  if (!parser.isSet(nosplash_option) && !(parser.isSet(loadfile_option) || parser.isSet(layout_option)))
  // if(false) // if you uncomment this line, a kitten will die somewhere in the world.
  {
    QPixmap main_pixmap;
    if ( parser.isSet(filesplash_option) )
    {
      main_pixmap   = QPixmap(parser.value(filesplash_option));  
    }
    else 
    {
      main_pixmap = getFunnySplashscreen();
    }

    QSplashScreen splash(main_pixmap, Qt::WindowStaysOnTopHint);
    QDesktopWidget* desktop = QApplication::desktop();
    const int scrn = desktop->screenNumber();
    const QPoint currentDesktopsCenter = desktop->availableGeometry(scrn).center();
    splash.move(currentDesktopsCenter - splash.rect().center());

    splash.show();
    app.processEvents();

    auto deadline = QDateTime::currentDateTime().addMSecs(500);
    while (QDateTime::currentDateTime() < deadline)
    {
      app.processEvents();
    }

    w = new MainWindow(parser);

    deadline = QDateTime::currentDateTime().addMSecs(1000);
    while (QDateTime::currentDateTime() < deadline && !splash.isHidden())
    {
      app.processEvents();
    }

    splash.finish(w);
  }
  else {
    w = new MainWindow(parser);
  }

  w->show();
  if ( parser.value(subscribe_option) == "true" ) {
    w->on_buttonStreamingStart_clicked();
  }
  return app.exec();
}
