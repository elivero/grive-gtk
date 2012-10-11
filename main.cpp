/*
 * grive-gtk v0.5.0 October 2012
 * by Bas Dalenoord, www.mijn.me.uk
 * 
 * Copyright (c) 2012 All rights reserved
 * 
 * Released under the BSD-license. A copy of the license should've been
 * shipped with the release, and can be found in the 'docs/license.txt' 
 * file you've unpacked from the downloaded archive.
 */

/*
 * FILE DESCRIPTION
 * 
 * File: 		main.cpp
 * Description:		Main source code file for the 'grive-gtk' project.
 * Last modification:	11 October 2012 by Bas Dalenoord
 */

/*
 * DEVELOPMENT NOTES
 * 
 * Compile using the following command:
 * 	g++ main.cpp -Os -s `pkg-config gtk+-2.0 --cflags` `pkg-config gtk+-2.0 --libs`
 */


//
// LIBRARY INCLUDES
//
#include <stdlib.h>			// Basic functions
#include <iostream>			// Input/Output stream
#include <string>			// C++ strings
#include <fstream>			// File stream
#include <sstream>			// String stream
#include <gtk/gtk.h>			// GTK library


//
// NAMESPACE DEFINITION
//
using namespace std;			// Using the standard namespace

//
// DEFINITIONS
//
#define	project_banner	"\n grive-gtk v0.5.0 October 2012\n by Bas Dalenoord, www.mijn.me.uk\n\n Copyright (c) 2012 All rights reserved\n\n Released under the BSD-license. A copy of the license should've been\n shipped with the release, and can be found in the 'docs/license.txt'\n file you've unpacked from the downloaded archive.\n\n"

//
// VARIABLE DECLARATIONS, SORTED BY TYPE
//
string		config_file	=	"/etc/grive-gtk.conf";				// Default location for the configuration, a custom location might've been defined in this file, after which this string will be altered
string		grive_directory	=	getenv("HOME") + string("/grive");		// Default location for the synchronized folder, a custom location might've been defined in the configuration file, after which this string will be altered
string		icon_file	=	"/usr/share/pixmaps/drive.png";			// Default location for grive-gtk's icon, a custom icon might've been defined in the configuration file, after which this string will be altered.
string		tooltip_text	=	"grive-gtk 0.5.0 by Bas Dalenoord";		// Default text of the tooltip which is displayed on icon hover

int		sync_timeout	=	600;	// Timeout for automatic synchronisation, default is 10 minutes (defined in seconds). A custom timeout might've been defined in the configuration file, after which this string will be altered

bool	debugger_enabled	=	true;	// Enables/Disables the debugger
bool	autoRetried	=	false;	// Has automatic resync been tried yet?

int retryCounter	=	0;	// Counter, holds the number of retries until now.

pid_t	pID;	// Child background process identifier 

//
// METHODS & CLASSES
//

// Method/Class declaration(s)
class debugger;	// Debugger class... Duh!
class configuration;	// Configuration class (You kiddin? :O)

string	timeString(string format = "%H:%M:%S");	// Format a time string to either a defined or a default format
string	exec(string* cmd);

bool hasEnding(string const &fullString, string const &endString);

void syncGrive();	// Synchronize the directory now
void displayInfo();
void destroyGriveGtk();	// Destroys all instances and processes related to grive-gtk

// Actual Class(es)
class debugger{
	/* 
	 * Debugger class, various functions used for debugging. Can be disabled by the boolean found in the definitions above
	 * Last modification: 7 October 2012 
	 */
	 
	public:
		static void throwError(string);
		static void throwWarning(string);
		static void throwSuccess(string);
		static void throwMessage(string);
};
void debugger::throwError(string message){ // Throws an error message in red
	if(debugger_enabled==true)
		cout << "\033[0;31m  " << timeString() << " >> grive-gtk >> Error >> " << message << "\033[0m\n\n";
}
void debugger::throwWarning(string message){ // Throws a warning message in orange
	if(debugger_enabled==true)
		cout << "\033[1;31m  " << timeString() << " >> grive-gtk >> Warning >> " << message << "\033[0m\n\n";
}
void debugger::throwSuccess(string message){ // Throws an Success message in green
	if(debugger_enabled==true)
		cout << "\033[0;32m  " << timeString() << " >> grive-gtk >> Success >> " << message << "\033[0m\n\n";
}
void debugger::throwMessage(string message){ // Throws a message in the default color of the local terminal
	if(debugger_enabled==true)
		cout << "  " << timeString() << " >> grive-gtk >> Message >> " << message << "\n\n";
}
class configuration{
	/*
	 * Configuration class, handles configuration files.
	 * Last modification: 7 October 2012
	 */
	 
	public:
		struct Config {};	// Data structure for the configuration file. The whole file will be loaded into this structure
		static string getValue(Config&, string);	// Load a specific value from the configuration file
};
string configuration::getValue(Config& config, string valueToLoad){
	string result;
	
	ifstream file_input(config_file.c_str());
	string line;
	while(getline(file_input,line)){
		istringstream sinput (line.substr(line.find("=")+1));
		if(line.find(valueToLoad) != -1 && line.find(string("#")) == -1){
			sinput >> result;
		}
	}
	
	if(result != "")
		return result;
	else
		return "";
}
class gtk{
	/*
	 * GTK class, holds the tray icon.
	 * Last modification: 8 October 2012
	 */
	 
	public:
		static GtkStatusIcon *createTrayIcon();	// Creates a new icon instance
		
		// Event handlers
		static void trayIconOnClick(GtkStatusIcon *tray_icon, gpointer user_data);
		static void trayIconOnMenu(GtkStatusIcon *tray_icon, guint button, guint activate_time, gpointer user_data);
};
void gtk::trayIconOnClick(GtkStatusIcon *tray_icon, gpointer user_data){
	debugger::throwMessage("Clicked on icon!");
}
void gtk::trayIconOnMenu(GtkStatusIcon *tray_icon, guint button, guint activate_time, gpointer user_data){
	static GtkWidget *menu = NULL;
	if(!menu){
		GtkWidget *item;
		menu = gtk_menu_new();
		
		item = gtk_menu_item_new_with_label("Synchronize my Drive");
		gtk_menu_append(menu,item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(syncGrive), GUINT_TO_POINTER(TRUE));
		
		item = gtk_menu_item_new(); // Separator
		gtk_menu_append(menu,item);
		
		item = gtk_menu_item_new_with_label("About this project");
		gtk_menu_append(menu,item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(displayInfo), GUINT_TO_POINTER(TRUE));
		
		item = gtk_menu_item_new_with_label("Quit grive-gtk");
		gtk_menu_append(menu,item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(destroyGriveGtk), GUINT_TO_POINTER(TRUE));
	}
	
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,gtk_status_icon_position_menu,tray_icon,button,activate_time);
}
GtkStatusIcon* gtk::createTrayIcon(){
	debugger::throwMessage("Creating a tray icon instance");
	
	GtkStatusIcon* tray_icon = gtk_status_icon_new();
	
	g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(trayIconOnClick), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(trayIconOnMenu), NULL);
	
	gtk_status_icon_set_from_file(tray_icon, icon_file.c_str());
	gtk_status_icon_set_tooltip(tray_icon, tooltip_text.c_str());
	
	gtk_status_icon_set_visible(tray_icon, TRUE);
	
	debugger::throwMessage("Tray icon instance created");
	return tray_icon;	
}

// Actual method(s)
string timeString(string format){
	/*
	 * Format a time_t into a human readable formatted string. Formatting will be done according to either the provided or default formatting string
	 * Last modification: 6 October 2012
	 */
	 
	string result;
	
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];
	
	time(&rawtime); // Load time into variable
	timeinfo = localtime(&rawtime); // Convert time to local settings
	
	strftime(buffer,80,format.c_str(),timeinfo); // Format to specified format
	
	result = buffer; // Convert it to a string...
	return result; // ... and return it!
}
string exec(string cmd){
	/*
	* Execute a command and place its output in a string variable for control purposes
	* Last modification: 10 October 2012
	*/
	FILE *fpipe;
	char line[512];
	string result = "";
	cmd = cmd.append(" 2>&1"); // Get messages from the stderr-stream as well... this also hides command output from the stdout... Nice!

	if(0 == (fpipe = (FILE*)popen(cmd.c_str(),"r"))){
		debugger::throwError("Could not execute command, popen() failed");
	}

	while(fgets(line, 512, fpipe)){
		result += line;
	}

	pclose(fpipe);

	return result;
}
bool hasEnding (string const &fullString, string const &endString){
	/*
	 * Function to check if a string ends with a certain supplied value, thanks to http://stackoverflow.com/questions/874134/
	 * Last revision: 10 October 2012
	 */
	if (fullString.length() >= endString.length()) {
		return (0 == fullString.compare (fullString.length() - endString.length(), endString.length(), endString));
	} else {
		return false;
	}
}

void syncGrive(){
	/*
	 * Synchronises the grive directory with Google Drive
	 * Last modification: 11 October 2012
	 */
	debugger::throwMessage("Grive synchronisation process initiated");
	
	// Check if reload of the directory path is required
	configuration::Config config;
	if(configuration::getValue(config, "GRIVE_DIRECTORY") != grive_directory){
		grive_directory = configuration::getValue(config, "GRIVE_DIRECTORY");
	}
	int	retryCount	=	atoi(configuration::getValue(config,"SYNC_AUTO_TIMES").c_str());	// Number of times to automatically retry
	
	// Check if the defined grive directory actually exists
	if(chdir(grive_directory.c_str()) != 0){ // The directory does not exist, throw an error
		debugger::throwError("The directory located at the defined grive directory path (" + grive_directory + ") could not be found. Probably a typo in your configuration...?");
	} else { // Directory exists, try syncing
		debugger::throwSuccess("Grive directory found, starting the sync process");
		string result = exec("grive"); // Execute the grive command and get it's output in a string for checking purpuses
		
		if(hasEnding(result,"Finished!\n") == 1) // Grive has syncronised succesfully	
			debugger::throwSuccess("Synchronisation finished");
		else{ // Synchronisation failed, throw an error...
			if(configuration::getValue(config, "SYNC_AUTO_RETRY") == "true" && autoRetried == false){
				while(retryCounter < retryCount){
					retryCounter++;
					debugger::throwError("Synchronisation failed due to an external error. Automatic retry " + retryCounter);
					syncGrive();
				}
				autoRetried = true;
			} else if(configuration::getValue(config, "SYNC_AUTO_RETRY") == "true" && autoRetried == true) {
				debugger::throwError("Synchronisation failed due to an external error. Try syncing manually...");
			}
		}
	}	
}
void displayInfo(){
	/*
	 * Displays an information dialog
	 * Last modification: 11 October 2012
	 */
	GtkWidget* popup = gtk_window_new(GTK_WINDOW_TOPLEVEL); // Create a new window instance
	gtk_window_set_title(GTK_WINDOW(popup),"About grive-gtk"); // Set the window title
	gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER); // Center the window on the screen
	
	GtkWidget* image = gtk_image_new_from_file(icon_file.c_str()); // Load the Drive logo
	GtkWidget* infoMesg = gtk_label_new("grive-gtk 0.5.0 October 2012\nby Bas Dalenoord, mijn.me.uk\n\nCopyright (c) 2012 All rights reserved\n\nReleased under the BSD-license. A copy of the license should've been\nshipped with the release, and can be found in the 'docs/license.txt'     \nfile you've unpacked from the downloaded archive."); // Create a text label
	GtkWidget* container = gtk_hbox_new(false,0); // Container which holds the elements
	
	gtk_widget_set_usize(image,128,128); // Resize the image object
	
	gtk_container_add(GTK_CONTAINER(container), image); // Add the image to the container
	gtk_container_add(GTK_CONTAINER(container),infoMesg); // Add the label to the container
	gtk_container_add(GTK_CONTAINER(popup),container); // Add the container to the window
	
	gtk_widget_show_all(popup); // Show the window...
}

void destroyGriveGtk(){
	/*
	 * Destroys the instance and all related processess to grive-gtk
	 * Last modification: 8 October 2012
	 */
	 
	debugger::throwMessage("Destroying grive-gtk (semi)peacefully...");	
	gtk_main_quit(); // Quit GTK
	kill(pID, SIGTERM); // Try to kill the child process
	if(pID){ // Assasin failed
		debugger::throwError("Child process could not be destroyed peacefully...");
		kill(pID, SIGKILL); // Kill it with a bigger gun
	}
	
	exit; // Exit peacefully
}

// Main method
int main(int argc, char* argv[]){
	/*
	 * Main method
	 * Last modification: 11 October 2012
	 */
	
	cout << project_banner; // Print a banner for the project
	
	// Try to load the grive directory from the configuration file
	configuration::Config config; grive_directory = configuration::getValue(config, "GRIVE_DIRECTORY");
	if(grive_directory != "") // Grive directory could be loaded properly, throws an Success
		debugger::throwSuccess("Defined grive directory: " + grive_directory);
	else{ // Grive directory could not be loaded... revert to default value, throws a warning
		grive_directory = getenv("HOME") + string("/grive");
		debugger::throwWarning("Grive directory undefined, reverting to default: " + grive_directory);
	}	
	// Check if the defined grive directory actually exists
	if(chdir(grive_directory.c_str()) != 0){ // The directory does not exist, throw an error
		debugger::throwError("The directory located at the defined grive directory path (" + grive_directory + ") could not be found. Probably a typo in your configuration...?");
	}
	
	// Try to create a new icon instance
	GtkStatusIcon* tray_icon;
	gtk_init(&argc, &argv);
	tray_icon = gtk::createTrayIcon();
	if(tray_icon != 0){ // Instance succesfully created
		pID = fork(); // Fork the icon process to the background
		if(pID == 0){
			while(pID == 0){
				debugger::throwSuccess("Icon process forked to the background");
				gtk_main();
			}
		} else if(pID < 0){
			debugger::throwError("Could not fork child process");
		}
	}
	else // Instance failed to create
		debugger::throwError("Could not create a new tray icon instance...");
}
