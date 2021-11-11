
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories( ./ ../  ../include )

QT5_ADD_RESOURCES (RES_SRC  resource.qrc )

# PJ_EXTRA_APP_RESOURCES may be used to compile in an additional (external) resource file
QT5_ADD_RESOURCES (RES_SRC_EXTRA ${PJ_EXTRA_APP_RESOURCES} )

QT5_WRAP_UI ( UI_SRC
    aboutdialog.ui
    mainwindow.ui

    support_dialog.ui
    curvelist_panel.ui
    preferences_dialog.ui
    suggest_dialog.ui
    new_release_dialog.ui

    plot_docker_toolbar.ui
    plotwidget_editor.ui

    transforms/function_editor.ui
    transforms/function_editor_help.ui
    transforms/transform_selector.ui

    transforms/first_derivative.ui
    transforms/moving_average_filter.ui
    transforms/outlier_removal.ui
    transforms/integral_transform.ui

    cheatsheet/cheatsheet_dialog.ui
    )

SET( PLOtJUGGLER_SRC

    cheatsheet/cheatsheet_dialog.cpp

    customtracker.cpp

    curvelist_panel.cpp
    curvelist_view.cpp
    curvetree_view.cpp
    dummy_data.cpp
    main.cpp
    mainwindow.cpp
    messageparser_base.cpp
    menubar.cpp
    new_release_dialog.cpp
#    plotlegend.cpp
    plotwidget.cpp
    plotwidget_editor.cpp
    plotwidget_transforms.cpp
    plot_docker.cpp
#    plotmagnifier.cpp
    preferences_dialog.cpp
    point_series_xy.cpp
#    plotzoomer.cpp

    suggest_dialog.cpp
#    timeseries_qwt.cpp
    tabbedplotwidget.cpp
    tab_widget.h
    tree_completer.h

    transforms/custom_function.cpp
    transforms/custom_timeseries.cpp
    transforms/function_editor.cpp
    transforms/transform_selector.cpp
    transforms/lua_custom_function.cpp
    transforms/moving_average_filter.cpp
    transforms/outlier_removal.cpp
    transforms/integral_transform.cpp

    transforms/first_derivative.cpp
    transforms/scale_transform.cpp

    utils.h
    utils.cpp

    mainwindow.h
    realslider.h

    nlohmann_parsers.cpp
    )

# This option enables the creation of a *.app MacOS application
if(PJ_ENABLE_MAC_APP)
    if (NOT APPLE)
        message(FATAL_ERROR
            "The option PJ_ENABLE_MAC_APP can only be set in MacOS systems")
    endif()

    set(ICON_NAME plotjuggler.icns)
    set(MAC_ICON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/resources/${ICON_NAME})

    # By providing a value on PLOTJUGGLER_MAC_PLIST, we can modify
    # the plist
    if(NOT DEFINED PLOTJUGGLER_MAC_PLIST)
        set(PLOTJUGGLER_MAC_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/macapp/plotjuggler.plist)
    endif()

    set_source_files_properties(${MAC_ICON_SRC}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(plotjuggler MACOSX_BUNDLE
        ${MAC_ICON_SRC}
        ${PLOtJUGGLER_SRC}
        ${RES_SRC}
        ${RES_SRC_EXTRA}
        ${UI_SRC}
        ${BACKWARD_ENABLE})

    set_target_properties(plotjuggler
        PROPERTIES
            BUNDLE True
            MACOSX_BUNDLE_GUI_IDENTIFIER io.plotjuggler.PlotJuggler
            MACOSX_BUNDLE_BUNDLE_NAME plotjuggler
            MACOSX_BUNDLE_INFO_PLIST ${PLOTJUGGLER_MAC_PLIST}
            MACOSX_BUNDLE_ICON_FILE ${ICON_NAME}
    )
else()
    add_executable(plotjuggler
        ${PLOtJUGGLER_SRC}
        ${RES_SRC}
        ${RES_SRC_EXTRA}
        ${UI_SRC}
        ${BACKWARD_ENABLE} )
endif()

if (NOT WIN32)
    add_backward(plotjuggler)
endif()

target_link_libraries(plotjuggler
    ${QT_LINK_LIBRARIES}
    colorwidgets
    lua_static
    qt_advanced_docking
    plotjuggler_base
    plotjuggler_qwt
    )

if(COMPILING_WITH_CATKIN)

    target_link_libraries(plotjuggler ${catkin_LIBRARIES} )

    install(TARGETS plotjuggler
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

elseif(COMPILING_WITH_AMENT)

    target_link_libraries(plotjuggler ${ament_index_cpp_LIBRARIES} )

    install(TARGETS plotjuggler
        DESTINATION lib/${PROJECT_NAME} )
else()
    install(TARGETS plotjuggler
        DESTINATION bin  )
endif()
