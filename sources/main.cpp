/*
 * grive-gtk
 * Graphical implementation of grive for Linux-based operating systems
 * v0.1.3 September 2012
 *
 * grive-gtk is a graphical frontend for the grive-project. grive aims
 * to be an open-source implementation of Google Drive.
 *
 * Copyright (c) 2012 by Bas Dalenoord.
 * 
 * Released under the BSD-license. A copy of the license should've been
 * shipped with the release, and can be found in the 'license.txt' file.
 */
 
/*
 * DEVELOPMENT NOTES
 * 
 * Compile using the following command:
 * 	g++ main.cpp -Os -s `pkg-config gtk+-2.0 --cflags` `pkg-config gtk+-2.0 --libs`
 */

//
// INCLUDES
//
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <gtk/gtk.h> 		// GTK-libraries (U kiddin?! xD)
using namespace std;

//
// DEFINITIONS
//

#define	 	config_file	 "/etc/grive-gtk.conf"							// Path to the configuration file for grive-gtk

//
// VARIABLE DECLARATIONS
//

string		grive_directory = getenv("HOME") + string("/grive");		// Path to the grive directory, defaulting to '/home/$USER/grive'
string		icon_file = "/usr/share/pixmaps/drive.png";					// Path to the default icon file for grive-gtk

string		tooltip = "grive-gtk | v0.1.3 | by Bas Dalenoord";			// Default tooltip text

GtkStatusIcon *tray_icon; 												// Define a new icon pointer

struct Config {}; 														// Struct for loading configuration files

//
// METHODS
//

class debugger;
string loadConfig(Config& config, string toLoad);
void trayIconOnClick(GtkStatusIcon *tray_icon, gpointer user_data);
void trayIconOnMenu(GtkStatusIcon *tray_icon, guint button, guint activate_time, gpointer user_data);
static GtkStatusIcon *createTrayIcon();
void syncGrive();
void showInfo();
void destroyGriveGtk();
void destroyThis();

/*
 * The debugger class can be used to execute various functions which make debugging easier.
 * Last revision: 25 September 2012
 */
class debugger
{
	public:
		static void throwError(string);		// Throws an error message (displayed in red)
		static void throwWarning(string);		// Throws a warning message (in blue)
		static void throwMessage(string); 		// Throws an notification message (in green)
		static void throwNotification(string);	// Throws a generic message (in gray)
};
void debugger::throwError(string errMesg){
	// Throw an error message (in red)
	cout << "\033[0;31m" << "  ERROR: " << errMesg << "\033[0m\n";
}
void debugger::throwWarning(string wrnMesg){	
	// Throw a warning message (in blue)
	cout << "\033[0;34m" << "  WARNING: " << wrnMesg << "\033[0m\n";
}
void debugger::throwMessage(string aprMesg){	
	// Throw an notification message (in green)
	cout << "\033[0;32m" << "  MESSAGE: " << aprMesg << "\033[0m\n";
}
void debugger::throwNotification(string notMesg){
	// Throw a generic message (in gray)
	cout << "\033[0;37m" << "  NOTIFICATION: " << notMesg << "\033[0m\n";
}

/*
 * Function to search through the configuration for a certain value
 * Last revision: 25 September 2012
 */
string loadConfig(Config& config, string toLoad){
	string result;
	
	ifstream finput (config_file); // Filestream
	string line; // Will contain the found result
	while(getline(finput,line)){
		istringstream sinput(line.substr(line.find("=") + 1)); // Stringstream
		if(line.find(toLoad) != -1 && line.find(string("#") + toLoad) == -1)
			sinput >> result;
	}
	
	if(result != "")
		return result;
	else
		return "";
}

/*
 * GTK Icon creation and handlers
 * Last revision: 25 September 2012
 */
void trayIconOnClick(GtkStatusIcon *tray_icon, gpointer user_data){
	debugger::throwNotification("Clicked on the tray icon");
}
void trayIconOnMenu(GtkStatusIcon *tray_icon, guint button, guint activate_time, gpointer user_data){
	//debugger::throwNotification("Right clicked on the tray icon");
	static GtkWidget *menu = NULL;
	if(!menu){
		GtkWidget *item;
		menu = gtk_menu_new();
		
		item = gtk_menu_item_new_with_label("Synchronize my Drive");
		gtk_menu_append(menu,item);
		g_signal_connect(G_OBJECT(item),"activate",G_CALLBACK(syncGrive),GUINT_TO_POINTER(TRUE));
		
		item = gtk_menu_item_new(); // Separator
		gtk_menu_append(menu,item);
		
		item = gtk_menu_item_new_with_label("Quit");
		gtk_menu_append(menu,item);
		g_signal_connect(G_OBJECT(item), "activate",G_CALLBACK(destroyGriveGtk), GUINT_TO_POINTER(TRUE));
		
		item = gtk_menu_item_new(); // Separator
		gtk_menu_append(menu,item);	
		
		item = gtk_menu_item_new_with_label("grive-gtk 0.1.3\nby Bas Dalenoord");
		gtk_widget_set_sensitive(item,false);
		gtk_menu_append(menu,item);	
	}
	
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,gtk_status_icon_position_menu,tray_icon,button,activate_time);
}
static GtkStatusIcon *createTrayIcon(){
	debugger::throwMessage("Creating the tray icon...");
	
	tray_icon = gtk_status_icon_new(); // Create a new icon instance
	g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(trayIconOnClick), NULL); // Left-click handler
	g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(trayIconOnMenu), NULL); // Right-click handler
	gtk_status_icon_set_from_file(tray_icon, icon_file.c_str()); // Load the icon from the file
	gtk_status_icon_set_tooltip(tray_icon, tooltip.c_str()); // Set the icon tooltip
	gtk_status_icon_set_visible(tray_icon, TRUE); // Make the icon visible
	
	debugger::throwMessage("Tray icon created!");
	return tray_icon;
}

/*
 * Synchronization function
 * Last revision: 25 September 2012
 */
void syncGrive(){
	debugger::throwMessage("Synchronization started...");
	
	Config config; 
	if(loadConfig(config,"GRIVE_DIRECTORY") != grive_directory) // If the grive-directory has been modified whilst running the process, change our path...
		grive_directory = loadConfig(config,"GRIVE_DIRECTORY");
		
	if(chdir(grive_directory.c_str()) == 0){
		system("grive");
		debugger::throwMessage("Synchronization finished!");
		gtk_status_icon_set_tooltip(tray_icon, "Synchronization finished!"); // Set the icon tooltip
	} else {
		debugger::throwError("Synchronization failed! Have you entered the path to your\n         Grive-directory correctly in '/etc/grive-gtk.conf'?");
		gtk_status_icon_set_tooltip(tray_icon, "Synchronization error..."); // Set the icon tooltip
	}
}

/*
 * Destroy our instance of grive-gtk
 * Last revision: 25 September 2012
 */
void destroyGriveGtk(){
	gtk_main_quit();	
	debugger::throwMessage("GTK killed, exit now...!");	
	exit;
}

//
// MAIN FUNCTION
//
int main(int argc, char* argv[]){	
	pid_t pID = fork();
	if(pID == 0){ // Child process
		debugger::throwMessage("Child process successfully forked!");
		
		// Load the variables from the grive-gtk configuration
		Config config; 
		if(loadConfig(config,"GRIVE_DIRECTORY") != "") // If the grive-directory is defined in the configuration file, load it. If not, the default location will be used.
			grive_directory = loadConfig(config,"GRIVE_DIRECTORY");
		debugger::throwMessage("Grive directory is located at " + grive_directory);
		
		if(loadConfig(config,"CUSTOM_ICON") != "") // If a custom icon location is defined in the configuration file, load it. If not, the default location will be used
			icon_file = loadConfig(config,"CUSTOM_ICON");
		debugger::throwMessage("Icon file is located at " + icon_file);
		
		GtkStatusIcon *tray_icon;
		gtk_init(&argc,&argv);
		tray_icon = createTrayIcon();
		gtk_main();
		
		return 0;
	} else if(pID < 0){ // Failed to fork!
		debugger::throwError("Failed to fork!");
	} else {
		cout << "\n grive-gtk\n Graphical implementation of grive for Linux-based operating systems\n v0.1.2 September 2012\n\n Copyright (c) 2012 by Bas Dalenoord\n\n"; // Print a banner
		debugger::throwNotification("Debugger started");
	}
	
	cout << "\n";
	return 0;
}
