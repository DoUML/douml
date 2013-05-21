# The directory where all the douml files will be installed, you
# can change it for instance because you don't have root permission
DOUML_LIB = /usr/lib/douml

# The directory where the douml shell script will be installed, you
# can change it for instance because you don't have root permission
DOUML_DIR = /usr/bin

# The directory where the doc will be installed, you
# can change it for instance because you don't have root permission
DOUML_DOC_DIR = /usr/share/doc/douml

# The directory where the doc will be installed, you
# can change it for instance because you don't have root permission
DOUML_LOCALE_DIR = ${DOUML_LIB}/locale

# The directory containing douml.desktop
# The copy is not done if you comment the definition
DOUML_DESKTOP_DIR = /usr/share/applications

# The directories containing the douml icons are
# <DOUML_ICONS_PREFIX_DIR>/<size>x<size>/apps
# The copy is not done if you comment the definition
DOUML_ICONS_PREFIX_DIR = /usr/share/icons/hicolor

# On several Unix systems, the icons used inside the
# menus must be stored inside the pixmaps' directory.
# The copy is done if you uncomment the definition.
DOUML_UNIX_PIXMAPS_DIR = /usr/share/pixmaps

# If not defined (local install), DoUML will be install into LOCAL_DESTDIR or "./install" directory
# Else (packaging) desktop and executable script are copied into DESTDIR but reference are /
ifndef DESTDIR
  ifndef LOCAL_DESTDIR
    LOCAL_DESTDIR = ${PWD}/install
  endif
  DESTDIR = ${LOCAL_DESTDIR}
endif

QMAKE=qmake

MAKE=make

ifndef config
  config=Release
endif

SRC_DIRS=\
	src/CppGenerator  \
	src/CppReverse  \
	src/CppRoundtrip  \
	src/IdlGenerator  \
	src/JavaCat  \
	src/JavaGenerator  \
	src/JavaReverse  \
	src/JavaRoundtrip  \
	src/PhpGenerator  \
	src/PhpReverse  \
	src/PythonGenerator  \
	src/RoundtripBody

PLUGOUT_DIRS=genplugouts/BrowserSort  \
	genplugouts/CppUtilities  \
	genplugouts/FileControl  \
	genplugouts/GlobalChange  \
	genplugouts/HtmlDoc  \
	genplugouts/Xmi2Generator  \
	genplugouts/xmi2import

PROGS=douml \
	browsersort  \
	cpp_generator  \
	cpp_reverse  \
	cpp_roundtrip  \
	cpp_util  \
	file_control  \
	ghtml  \
	global_change \
	idl_generator  \
	java_catalog  \
	java_generator  \
	java_reverse  \
	java_roundtrip  \
	php_generator  \
	php_reverse  \
	python_generator  \
	roundtrip_body  \
	gxmi2  \
	ixmi2

compile: src/settings_ini.h
	( cd src/Libs/L_UniversalModels ; $(QMAKE) -config ${config} L_UniversalModels.pro; ) || exit 1 ; $(MAKE) -C src/Libs/L_UniversalModels || exit 1
	( cd src ; $(QMAKE) -config ${config} douml.pro; ) || exit 1 ; $(MAKE) -C src || exit 1
	for i in $(SRC_DIRS); do if [ -d $$i ]; then ( cd $$i; $(QMAKE) -config ${config}; ) || exit 1 ; $(MAKE) -C $$i || exit 1 ; fi; done
	for i in $(PLUGOUT_DIRS); do if [ -d $$i ]; then ( cd $$i; $(QMAKE) -config ${config}; ) || exit 1 ; $(MAKE) -C $$i || exit 1 ; fi; done

src/settings_ini.h: bin/settings.ini
	( cd bin && xxd -i  settings.ini ../src/settings_ini.h; ) || exit 1

install:
	mkdir -p "$(DESTDIR)$(DOUML_DIR)"
	if test -n "$(DOUML_ICONS_PREFIX_DIR)" ; \
	then \
		for i in 16 32 48 64; do \
			mkdir -p "$(DESTDIR)$(DOUML_ICONS_PREFIX_DIR)/$$i"x"$$i/apps"; \
			cp -p douml.$$i.png "$(DESTDIR)$(DOUML_ICONS_PREFIX_DIR)/$$i"x"$$i/apps/douml.png"; \
		done; \
	fi
	if test -n "$(DOUML_UNIX_PIXMAPS_DIR)"; then \
		mkdir -p "$(DESTDIR)$(DOUML_UNIX_PIXMAPS_DIR)"; \
		cp -f douml.48.png "$(DESTDIR)$(DOUML_UNIX_PIXMAPS_DIR)/douml.png"; \
	fi
	mkdir -p "$(DESTDIR)$(DOUML_LIB)"
	cp -Rp plugouts/empty "$(DESTDIR)$(DOUML_LIB)"
	mkdir -p "$(DESTDIR)$(DOUML_DOC_DIR)"
	mkdir -p "$(DESTDIR)$(DOUML_LOCALE_DIR)"
	cp -p *.lang "$(DESTDIR)$(DOUML_LOCALE_DIR)"
	for i in $(PROGS); do cp -p bin/$$i "$(DESTDIR)$(DOUML_LIB)" ; done
	echo "#!/bin/sh" >$(DESTDIR)$(DOUML_DIR)/douml
	echo "PATH=$(LOCAL_DESTDIR)$(DOUML_LIB):$$"PATH >>$(DESTDIR)$(DOUML_DIR)/douml
	echo "DOUML_LIB_DIR=$(LOCAL_DESTDIR)$(DOUML_LIB)" >>$(DESTDIR)$(DOUML_DIR)/douml
	echo "export PATH" >>$(DESTDIR)$(DOUML_DIR)/douml
	echo "export DOUML_LIB_DIR" >>$(DESTDIR)$(DOUML_DIR)/douml
	echo "exec $(LOCAL_DESTDIR)$(DOUML_LIB)/douml \"$$"@"\"" >>$(DESTDIR)$(DOUML_DIR)/douml
	chmod +x "$(DESTDIR)$(DOUML_DIR)/douml"
	if test -n "$(DOUML_DESKTOP_DIR)" ; \
	then \
		mkdir -p "$(DESTDIR)$(DOUML_DESKTOP_DIR)" ; \
		echo "[Desktop Entry]" > "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Encoding=UTF-8" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Name=DoUML" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Type=Application" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "GenericName=Free UML 2 modeler" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Comment=Free UML 2 modeler" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Exec=$(LOCAL_DESTDIR)$(DOUML_DIR)/douml" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "TryExec=douml" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Icon=douml" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
		echo "Categories=Development;Building;GUIDesigner;IDE;Documentation;" >> "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop" ; \
	fi

uninstall:
	rm -rf $(DESTDIR)$(DOUML_LIB) $(DESTDIR)$(DOUML_DIR)/douml
	for i in 16 32 48 64; do \
		rm -f "$(DESTDIR)$(DOUML_ICONS_PREFIX_DIR)/$$i"x"$$i/apps/douml.png"; \
	done
	rm -f "$(DESTDIR)$(DOUML_UNIX_PIXMAPS_DIR)/douml.png"
	rm -f "$(DESTDIR)$(DOUML_DESKTOP_DIR)/douml.desktop"

clean:
	for i in $(SRC_DIRS) $(PLUGOUT_DIRS); do if [ -d $$i ]; then (cd $$i; $(QMAKE) -config ${config}; $(MAKE) clean; rm -f Makefile; ) || exit 1 ; fi; done
	( cd src ; $(QMAKE) -config ${config} douml.pro; ) || exit 1 ; (cd src; $(MAKE) clean; rm -f Makefile; ) || exit 1
	for i in $(PROGS); do rm -f bin/$$i; done
	rm -rf bin/moc_release bin/moc_debug
	rm -rf bin/obj_release bin/obj_debug

.PHONY: compile clean install uninstall
