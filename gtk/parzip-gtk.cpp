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

struct app {
    GtkWidget *win;
    GtkWidget *box;
    GtkWidget *filemenu;
    GtkWidget *menubar;
    GtkWidget *treeview;
    GtkTreeStore *treestore;
};

enum ViewColumns {
    NAME_COLUMN,
    PACKED_SIZE_COLUMN,
    N_COLUMNS,
};

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
        printf("%s\n", filename);
        g_free(filename);
    }
    gtk_widget_destroy(fc);
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
    auto quit = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(a.filemenu), open);
    g_signal_connect(open, "activate", G_CALLBACK(open_file), &a);
    gtk_menu_shell_append(GTK_MENU_SHELL(a.filemenu), quit);
    g_signal_connect(quit, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(a.menubar), fmenu);
    gtk_box_pack_start(GTK_BOX(a.box), a.menubar, FALSE, FALSE, 0);

    // Treeview.
    a.treestore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_INT64);
    a.treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(a.treestore));
    gtk_tree_view_append_column(GTK_TREE_VIEW(a.treeview),
            gtk_tree_view_column_new_with_attributes("Filename",
                    gtk_cell_renderer_text_new(), "text", NAME_COLUMN, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(a.treeview),
            gtk_tree_view_column_new_with_attributes("Packed size",
                    gtk_cell_renderer_text_new(), "text", PACKED_SIZE_COLUMN, nullptr));
    GtkTreeIter i;
    gtk_tree_store_append(a.treestore, &i, nullptr);
    gtk_tree_store_set(a.treestore, &i, NAME_COLUMN, "Some filename", PACKED_SIZE_COLUMN, (int64_t)10, -1);
    gtk_box_pack_start(GTK_BOX(a.box), a.treeview, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(a.win), a.box);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    app a;
    buildgui(a);
    gtk_widget_show_all(a.win);
    gtk_main();
    return 0;
}
