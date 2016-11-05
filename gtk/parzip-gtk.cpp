/*
 * Copyright (C) 2016 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include<gtk/gtk.h>

#include<memory>

#include"zipfile.h"

struct app {
    GtkWidget *win;
    GtkWidget *box;
    GtkWidget *filemenu;
    GtkWidget *menubar;
    GtkWidget *treeview;
    GtkWidget *unpack_dialog;
    GtkWidget *unpack_progress;
    GtkTreeStore *treestore;
    std::unique_ptr<ZipFile> zfile;
    TaskControl *tc;
};

enum ViewColumns {
    NAME_COLUMN,
    PACKED_SIZE_COLUMN,
    UNPACKED_SIZE_COLUMN,
    N_COLUMNS,
};

void fill_recursively(app *a, const DirectoryDisplayInfo *di, GtkTreeIter *parent) {
    for(const auto &d : di->dirs) {
        GtkTreeIter i;
        gtk_tree_store_append(a->treestore, &i, parent);
        gtk_tree_store_set(a->treestore, &i,
                NAME_COLUMN, d.dirname.c_str(),
                PACKED_SIZE_COLUMN, 0,
                UNPACKED_SIZE_COLUMN, 0,
                -1);
        fill_recursively(a, &d, &i);
    }
    for(const auto &f : di->files) {
        GtkTreeIter i;
        gtk_tree_store_append(a->treestore, &i, parent);
        gtk_tree_store_set(a->treestore, &i,
                NAME_COLUMN, f.fname.c_str(),
                PACKED_SIZE_COLUMN, f.compressed_size,
                UNPACKED_SIZE_COLUMN, f.uncompressed_size,
                -1);
    }
}

void reset_model(app *a) {
    gtk_tree_store_clear(a->treestore);
    auto ziptree = a->zfile->build_tree();
    fill_recursively(a, &ziptree, nullptr);
}

void open_file(GtkMenuItem *, gpointer data) {
    app *a = reinterpret_cast<app*>(data);
    GtkWidget *fc = gtk_file_chooser_dialog_new("Select ZIP file", GTK_WINDOW(a->win),
                GTK_FILE_CHOOSER_ACTION_OPEN,
                "_Cancel", GTK_RESPONSE_CANCEL,
                "_Open", GTK_RESPONSE_ACCEPT,
                nullptr);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.zip");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fc), filter);
    auto res = gtk_dialog_run(GTK_DIALOG(fc));
    if(res == GTK_RESPONSE_ACCEPT) {
        auto filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
        a->zfile.reset(new ZipFile(filename));
        g_free(filename);
        reset_model(a);
    }
    gtk_widget_destroy(fc);
}

gboolean unpack_timeout_func(gpointer data) {
    app *a = reinterpret_cast<app*>(data);
    auto finished = a->tc->finished();
    auto total = a->tc->total();
    if(a->tc->state() != TASK_RUNNING) {
        gtk_widget_hide(a->unpack_dialog);
        return FALSE;
    }
    double fraction = ((double) finished) / total;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(a->unpack_progress), fraction);
    return TRUE;
}

void unpack_current(GtkMenuItem *, gpointer data) {
    app *a = reinterpret_cast<app*>(data);
    std::string unpack_dir;
    int num_threads = std::max((int)std::thread::hardware_concurrency(), 1);
    if(!a->zfile) {
        return;
    }
    GtkWidget *dc = gtk_file_chooser_dialog_new("Output directory",
            GTK_WINDOW(a->win),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "_Cancel",
            GTK_RESPONSE_CANCEL,
            "_Open",
            GTK_RESPONSE_ACCEPT,
            nullptr);
    auto res = gtk_dialog_run(GTK_DIALOG(dc));
    if(res == GTK_RESPONSE_ACCEPT) {
        char *dirname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dc));
        gtk_widget_destroy(dc);
        unpack_dir = dirname;
        g_free(dirname);
    } else {
        gtk_widget_destroy(dc);
        return;
    }
    a->tc = a->zfile->unzip(unpack_dir, num_threads);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(a->unpack_progress), 0);
    gtk_widget_show_all(a->unpack_dialog);
    g_timeout_add(500, unpack_timeout_func, data);
}

void stop_unpack(GtkDialog *, gint, gpointer data) {
    app *a = reinterpret_cast<app*>(data);
    a->tc->stop();
}


void buildgui(app &a) {
    a.win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(a.win), "Parzip");
    gtk_window_set_default_size(GTK_WINDOW(a.win), 640, 480);
    g_signal_connect(a.win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    a.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Menus
    a.menubar = gtk_menu_bar_new();
    a.filemenu = gtk_menu_new();
    GtkWidget *fmenu = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fmenu), a.filemenu);
    auto open = gtk_menu_item_new_with_label("Open");
    auto unpack = gtk_menu_item_new_with_label("Unpack");
    auto quit = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(a.filemenu), open);
    g_signal_connect(open, "activate", G_CALLBACK(open_file), &a);
    gtk_menu_shell_append(GTK_MENU_SHELL(a.filemenu), unpack);
    g_signal_connect(unpack, "activate", G_CALLBACK(unpack_current), &a);
    gtk_menu_shell_append(GTK_MENU_SHELL(a.filemenu), quit);
    g_signal_connect(quit, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(a.menubar), fmenu);
    gtk_box_pack_start(GTK_BOX(a.box), a.menubar, FALSE, FALSE, 0);

    // Treeview.
    a.treestore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_INT64, G_TYPE_INT64);
    a.treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(a.treestore));
    gtk_tree_view_append_column(GTK_TREE_VIEW(a.treeview),
            gtk_tree_view_column_new_with_attributes("Filename",
                    gtk_cell_renderer_text_new(), "text", NAME_COLUMN, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(a.treeview),
            gtk_tree_view_column_new_with_attributes("Packed size",
                    gtk_cell_renderer_text_new(), "text", PACKED_SIZE_COLUMN, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(a.treeview),
            gtk_tree_view_column_new_with_attributes("Unpacked size",
                    gtk_cell_renderer_text_new(), "text", UNPACKED_SIZE_COLUMN, nullptr));
    GtkWidget *scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(scroll), a.treeview);
    gtk_box_pack_start(GTK_BOX(a.box), scroll, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(a.win), a.box);

    // Unpack progress bar dialog.
    a.unpack_progress = gtk_progress_bar_new();

    a.unpack_dialog = gtk_dialog_new_with_buttons("Unpacking Zip file",
            GTK_WINDOW(a.win),
            GTK_DIALOG_MODAL,
            "Cancel",
            GTK_RESPONSE_REJECT,
            nullptr);
    g_signal_connect(a.unpack_dialog, "response", G_CALLBACK(stop_unpack), &a);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(a.unpack_dialog))),
            a.unpack_progress, TRUE, TRUE, 0);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    app a;
    buildgui(a);
    gtk_widget_show_all(a.win);
    gtk_main();
    return 0;
}
