Built from tag: 1.0b7

DoUML version 1.0b7
==================

*** IMPORTANT NOTICE ***

This is still a work in progress and the fact that the release is made doesn't mean that it is bug free. We recommend that you use DoUML along with source control and save your work frequently.

___________________

About
-----

 
 DoUML is a UML diagram editor capable of generating/reversing code in a multitude of programming
 languages(namely -  C++, Java, Python, idl, Php).
 
 DoUML is a fork of the original Bouml editor by Bruno Pages that was born when Bouml's development
 abruptly came to  a stop due to author's conflict with wikipedia administration.
 
 Not wanting to lose (possibly) best free UML modelling software an effort was made to port the
 original to Qt4 and continue developing it as a community.
 
 DoUML is based on a source code of Bouml 4.22 but can read projects saved with versions up to 4.23
 patch 7. Support for later, commercial versions, of Bouml is possible but sample projects will be
 required.
	
Project status:
--------------

 * Core editor seems to be working. It is still very rough around the edges but much easier to work
   with than bouml 4.23 (I am biased, lol)
 * Generators/reversers seem to be working.
 * Some of the plugouts do not work yet, but these are not critical to actual modelling.
 * A list of failing plugouts can be found in settings.ini file

Bugfixes:
---------

 * clicking left-right arrows when editing a parameter in an operation dialog will no longer switch
   table cells.
 * when you are inheriting from a base class, a definition is automatically produced even if it was 
   completely empty in base class
 * changing a default value in Uml tab of an operation dialog will automatically refresh function
   declaration in C++ tab.
 * user aliases are no longer lost when operation is copied or when "deploy classes" is used.
	
Enchancements:
--------------
		
   * In Bouml when .lock file was not deleted during previous session the editor notified the user
     of this fact and closed. Now it will offer the possibility to take ownership of that lock and
     continue.
		
   * Operation and class dialogs have "up/down" buttons so user can traverse the tree without
     reopening the dialogs.  This will later work for other dialogs too but making dialogs
     compatible with the feature is very time consuming so there's still time before that.
		
   * Operation, class and artifact dialogs can be moved around with a new sticky control appearing
    at wherever cursor leaves the dialog. A mini toolbar will appear wherever cursor touches the 
    edge of the dialog. This toolbar will have mouse icon on it. 
      * Left click-drag on this icon - drags the window around
      * Right click-drag on this icon - resizes the winow
      * Scrolling on this icon switches tabs of the dialog	
		
   * entering constructor initializers for C++ classes no longer requires navigation to properties
     panel. Right clicking on a constructor produces a menu entry "Add constructor initializer"
     which calls a dialog designed just for this purpose.
		
   * changes to inherited functions propatate above/below through the hierachy. User no longer need
     to manually adjust the whole tree each time.
	   
   * added C++ modifiers default,delete,override, final to operations
		
   * added non-static data initialization for C++ headers.  Basically - "initial value" now works
     for non-static data attributes.
		
   * "Up" right click menu option will appear in operation's right click menu. This option will
      instantly select and scroll to container class.
		
   * operations, class and artifact dialogs have clipboard history button on their mini toolbar.
     By clicking it user has access to recently copied chuncks of text.

   * when adding inherited functions from a base class (right click on a class node) user can now
     inherit whole inteface of a base class in one click.
		
   * all dialogs that have mini toolbar next to them will spawn near the cursor instead of the
     center of the screen.

   * user can now "move into", "move after" and "duplicate" into a parameter that has Get/Set
     functions. The operation also copies/moves getters/setters.
		
   * getters/setters can now be marked and "move after" within the same class
		
   * non-english characters are now allowed in comments, init values of attributes, notes on
      diagrams and all non-generatable nodes in the tree(i.e. - use cases). 
		
   * there is now "Generate" label on the main toolbar. 
     Next to this label C++/Php/Python... buttons will appear according to which language is set for
     the project clicking on these buttons will either generate code for currently selected node in
     the tree or marked nodes in the tree. To make it obvious which will be applied "Generate" label
     is color-coded. 
      * If its background is red - there are marked items in the tree and their nodes will be generated
      * If its background is blue - there are no marked in the tree so generation will be done on user-selected node
      * The generation is done on a "first generatable node upwards in the tree" Meaning - if an attribute is marked the app will traverse to containing class and generate it
   * Added "Catalog" widget below class tree. This widget is used for quick tree navigation and has 3 tabs.
      * "Visited" tab updates each time user clicks on a node in a tree. This node is inserted into visited and clicking on it will immediately scroll the main tree to it.
      * "Marked" tab tracks user selection and serves double purpose of tree navigation(as in "Visited") and node deselection via rightclick menu.
      * "Favourites" tab is used when user ahs often visited tree nodes and wants an easy wasy to return to the in a tree.
      * Adding nodes s done by dragging them from the main tree onto favourites tabs.
      * Catalog widget has line edit under tabs which serves the purpose of filtering the Visited view (only Visited atm).
      
   * Builtin types" tab in generation settings has been completely reworked. 
      * User can now search and sort it.
      * User can selectively turn off languages he does not use in the table.
      * When a new row is being added it is initialized with enough defaults to make assigning UML name the only obligatory edit before clicking "OK".
      * Adding new rows is now done through "Add type" button and a few options were removed from "do" menu as they no longer make sense.

   * A totally new control can be brought up by pressing Ctlr-E on tree nodes. See changelog.txt for details. (section 1.0b4) 
      
Manual
------

  Currently - the manual of Bouml version 4.22 is being used. To access it, set "Manual" path in
  miscelaneous settings in application's menu.

Compiling
---------

#### Under Linux:
  
 * dependencies: libqt4-dev, libqt4-qt3support, libboost-dev, libfile-fcntllock-perl, vim-common, g++ (>=4.7)
 * `make`
 * `make install` default directory is ${PWD}/install or `make install LOCAL_DESTDIR=[path]`
    
  
#### Under Windows:

  ```
  L_UniversalModels library needs to be built prior to douml compilation. It is located in src/Libs/L_UniversalModels
  ```

 * Mingw based on gcc 4.7+
 * Qt 4.7+ built with aforementioned mingw.
 * Boost's headers of version 1.5+ must be visible in scope.

	
Webpage
-------
  http://sourceforge.net/projects/douml/

Contact email: 
-------------
  doumleditor@gmail.com
  
  *** CONTRIBUTORS NEEDED ***		
	
And remember - we all own a debt of gratitude to Bouml's original creator - Bruno Pages. 
Without him, there'd be nothing for us to continue developing.
Current (non-free) version of original Bouml that he works on now can be found at:
	http://bouml.fr
