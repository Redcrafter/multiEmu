#ifdef __linux
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <gtk-2.0/gtk/gtk.h>
#include "nfd.h"

const char INIT_FAIL_MSG[] = "gtk_init_check failed to initilaize GTK+";
using namespace NFD;

static void AddTypeToFilterName(const char* typebuf, char* filterName, size_t bufsize) {
	const char SEP[] = ", ";

	size_t len = strlen(filterName);
	if(len != 0) {
		strncat(filterName, SEP, bufsize - len - 1);
		len += strlen(SEP);
	}

	strncat(filterName, typebuf, bufsize - len - 1);
}

static void AddFiltersToDialog(GtkWidget* dialog, const char* filterList) {
	GtkFileFilter* filter;
	char typebuf[NFD_MAX_STRLEN] = {0};
	const char* p_filterList = filterList;
	char* p_typebuf = typebuf;
	char filterName[NFD_MAX_STRLEN] = {0};

	if(!filterList || strlen(filterList) == 0)
		return;

	filter = gtk_file_filter_new();
	while(true) {
		if(NFDi_IsFilterSegmentChar(*p_filterList)) {
			char typebufWildcard[NFD_MAX_STRLEN];
			/* add another type to the filter */
			assert(strlen(typebuf) > 0);
			assert(strlen(typebuf) < NFD_MAX_STRLEN-1);

			snprintf(typebufWildcard, NFD_MAX_STRLEN, "*.%s", typebuf);
			AddTypeToFilterName(typebuf, filterName, NFD_MAX_STRLEN);

			gtk_file_filter_add_pattern(filter, typebufWildcard);

			p_typebuf = typebuf;
			memset(typebuf, 0, sizeof(char) * NFD_MAX_STRLEN);
		}

		if(*p_filterList == ';' || *p_filterList == '\0') {
			/* end of filter -- add it to the dialog */

			gtk_file_filter_set_name(filter, filterName);
			gtk_file_chooser_add_filter(dialog, filter);

			filterName[0] = '\0';

			if(*p_filterList == '\0')
				break;

			filter = gtk_file_filter_new();
		}

		if(!NFDi_IsFilterSegmentChar(*p_filterList)) {
			*p_typebuf = *p_filterList;
			p_typebuf++;
		}

		p_filterList++;
	}

	/* always append a wildcard option to the end*/

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "*.*");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(dialog, filter);
}

static void SetDefaultPath(GtkWidget* dialog, const char* defaultPath) {
	if(!defaultPath || strlen(defaultPath) == 0)
		return;

	/* GTK+ manual recommends not specifically setting the default path.
	   We do it anyway in order to be consistent across platforms.

	   If consistency with the native OS is preferred, this is the line
	   to comment out. -ml */
	gtk_file_chooser_set_current_folder(dialog, defaultPath);
}

static void WaitForCleanup() {
	while(gtk_events_pending())
		gtk_main_iteration();
}

/* public */

Result NFD::OpenDialog(const char* filterList, const char* defaultPath, std::string& outPath) {
	GtkWidget* dialog;
	Result result;

	if(!gtk_init_check(NULL, NULL)) {
		NFDi_SetError(INIT_FAIL_MSG);
		return Result::Error;
	}

	dialog = gtk_file_chooser_dialog_new("Open File",
	                                     NULL,
	                                     GTK_FILE_CHOOSER_ACTION_OPEN,
	                                     "_Cancel", GTK_RESPONSE_CANCEL,
	                                     "_Open", GTK_RESPONSE_ACCEPT,
	                                     NULL);

	/* Build the filter list */
	AddFiltersToDialog(dialog, filterList);

	/* Set the default path */
	SetDefaultPath(dialog, defaultPath);

	result = Result::Cancel;
	if(gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT) {
		char* filename;

		filename = gtk_file_chooser_get_filename(dialog);

		{
			size_t len = strlen(filename);
			*outPath = NFDi_Malloc(len + 1);
			memcpy(*outPath, filename, len + 1);
			if(!*outPath) {
				g_free(filename);
				gtk_widget_destroy(dialog);
				return Result::Error;
			}
		}
		g_free(filename);

		result = Result::Okay;
	}

	WaitForCleanup();
	gtk_widget_destroy(dialog);
	WaitForCleanup();

	return result;
}

Result NFD::OpenDialogMultiple(const char* filterList, const char* defaultPath, std::vector<std::string>& outPaths) {
	GtkWidget* dialog;
	Result result;

	if(!gtk_init_check(NULL, NULL)) {
		NFDi_SetError(INIT_FAIL_MSG);
		return Result::Error;
	}

	dialog = gtk_file_chooser_dialog_new("Open Files", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple(dialog, TRUE);

	/* Build the filter list */
	AddFiltersToDialog(dialog, filterList);

	/* Set the default path */
	SetDefaultPath(dialog, defaultPath);

	if(gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT) {
		GSList* fileList = gtk_file_chooser_get_filenames(dialog);

		GSList* node;

		/* fill buf */
		for(node = fileList; node; node = node->next) {
			outPaths.push_back(node->data);
			g_free(node->data);
		}

		g_slist_free(fileList);
		result = Result::Okay;
	} else {
		result = Result::Cancel;
	}

	WaitForCleanup();
	gtk_widget_destroy(dialog);
	WaitForCleanup();

	return result;
}

Result NFD::SaveDialog(const char* filterList, const char* defaultPath, std::string& outPath) {
	GtkWidget* dialog;
	Result result;

	if(!gtk_init_check(NULL, NULL)) {
		NFDi_SetError(INIT_FAIL_MSG);
		return Result::Error;
	}

	dialog = gtk_file_chooser_dialog_new("Save File",
	                                     NULL,
	                                     GTK_FILE_CHOOSER_ACTION_SAVE,
	                                     "_Cancel", GTK_RESPONSE_CANCEL,
	                                     "_Save", GTK_RESPONSE_ACCEPT,
	                                     NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(dialog, TRUE);

	/* Build the filter list */
	AddFiltersToDialog(dialog, filterList);

	/* Set the default path */
	SetDefaultPath(dialog, defaultPath);

	result = Result::Cancel;
	if(gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT) {
		char* filename;
		filename = gtk_file_chooser_get_filename(dialog);

		{
			size_t len = strlen(filename);
			*outPath = NFDi_Malloc(len + 1);
			memcpy(*outPath, filename, len + 1);
			if(!*outPath) {
				g_free(filename);
				gtk_widget_destroy(dialog);
				return Result::Error;
			}
		}
		g_free(filename);

		result = Result::Okay;
	}

	WaitForCleanup();
	gtk_widget_destroy(dialog);
	WaitForCleanup();

	return result;
}

Result NFD_PickFolder(const char* defaultPath, std::string& outPath) {
	GtkWidget* dialog;
	Result result;

	if(!gtk_init_check(NULL, NULL)) {
		NFDi_SetError(INIT_FAIL_MSG);
		return Result::Error;
	}

	dialog = gtk_file_chooser_dialog_new("Select folder",
	                                     NULL,
	                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                     "_Cancel", GTK_RESPONSE_CANCEL,
	                                     "_Select", GTK_RESPONSE_ACCEPT,
	                                     NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(dialog, TRUE);


	/* Set the default path */
	SetDefaultPath(dialog, defaultPath);

	result = Result::Cancel;
	if(gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT) {
		char* filename;
		filename = gtk_file_chooser_get_filename(dialog);

		{
			size_t len = strlen(filename);
			*outPath = NFDi_Malloc(len + 1);
			memcpy(*outPath, filename, len + 1);
			if(!*outPath) {
				g_free(filename);
				gtk_widget_destroy(dialog);
				return Result::Error;
			}
		}
		g_free(filename);

		result = Result::Okay;
	}

	WaitForCleanup();
	gtk_widget_destroy(dialog);
	WaitForCleanup();

	return result;
}
#endif
