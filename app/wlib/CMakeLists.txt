PROJECT(wlib)

INCLUDE_DIRECTORIES("${wlib_SOURCE_DIR}/include")

IF(XTRKCAD_USE_GTK)
	ADD_SUBDIRECTORY(gtklib)
ELSE(XTRKCAD_USE_GTK)
	ADD_SUBDIRECTORY(mswlib)
ENDIF(XTRKCAD_USE_GTK)

