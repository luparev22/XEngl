#include "vocabulary.h"

void read_from_bfile(GtkListStore* model)
{
    /* Чтение из файла */
    struct Item* item;
    item = (struct Item*)malloc(sizeof(struct Item));
    GtkTreeIter iter;
    FILE* file = fopen("data/voc.dat", "rb");
    if (file == NULL) {
        file = fopen("data/voc.dat", "wb");
        file = def_words(file);
        fclose(file);
        file = fopen("data/voc.dat", "rb");
    }
    fseek(file, sizeof(struct Item) * NUM_DEF_WORDS, 0);
    fread(item, sizeof(struct Item), 1, file);
    while (file != NULL && !feof(file)) {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, 0, item->word, 1, item->translation, -1);
        gtk_main_iteration_do(gtk_events_pending());
        fread(item, sizeof(struct Item), 1, file);
    }
    fclose(file);
    file = NULL;
}

gint sortsave(GtkTreeModel* model, GtkTreeIter* a, GtkTreeIter* b, gpointer user_data)
{
    gtk_main_iteration_do(gtk_events_pending());
    gchar *first, *second;
    gtk_tree_model_get(model, a, 0, &first, -1);
    gtk_tree_model_get(model, b, 0, &second, -1);
    gint return_value = g_utf8_collate(first, second);
    g_free(first);
    g_free(second);
    return return_value;
}
void write_to_bfile(GtkWidget* button, gpointer data)
{
    GtkWidget* window = gtk_widget_get_toplevel(button);
    g_signal_handlers_disconnect_by_func(G_OBJECT(window), key_press_event_voc, NULL);
    GtkWidget* main_overlay = g_object_ref((GtkWidget*)gtk_bin_get_child(GTK_BIN(window)));
    GtkWidget* main_box = g_object_ref((GtkWidget*)gtk_bin_get_child(GTK_BIN(main_overlay)));
    gtk_container_remove(GTK_CONTAINER(main_overlay), main_box);

    GtkWidget* spin_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_overlay), spin_box);

    /* Спиннер */
    GtkWidget* spinner = gtk_spinner_new();
    gtk_box_pack_end(GTK_BOX(spin_box), spinner, TRUE, TRUE, 30);
    gtk_widget_show_all(spin_box);
    gtk_spinner_start(GTK_SPINNER(spinner));
    gtk_main_iteration_do(gtk_events_pending());

    GtkTreeView* treeview = (GtkTreeView*)data;
    GtkTreeModel* model = gtk_tree_view_get_model(treeview);

    GtkTreeSortable* sortable = GTK_TREE_SORTABLE(model);
    /* Отмена сортировки */
    gtk_tree_sortable_set_sort_column_id(sortable, -2, 1);

    /* Сортировка по алфавиту */
    gtk_tree_sortable_set_sort_func(sortable, 0, sortsave, GINT_TO_POINTER(0), NULL);
    gtk_tree_view_column_clicked(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), 0));

    GtkTreeIter iter;
    gboolean valid;
    struct Item item;
    valid = gtk_tree_model_get_iter_first(model, &iter);
    FILE* file;
    gchar* word;
    gchar* translation;
    file = fopen("data/temp.dat", "wb"); // открытие бинарного файла для записи
    file = def_words(file);
    while (valid) {
        gtk_main_iteration_do(gtk_events_pending());
        gtk_tree_model_get(model, &iter, 0, &(word), 1, &(translation), -1);
        if (!((g_utf8_collate(word, "'Слово'") == 0) || (g_utf8_collate(translation, "'Перевод'") == 0))) {
            strcpy(item.word, word);
            strcpy(item.translation, translation);
            fwrite(&item, sizeof(item), 1, file); // запись в файл структуры
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    fclose(file);
    remove("data/voc.dat");
    rename("data/temp.dat", "data/voc.dat");

    gtk_widget_destroy(spinner);
    gtk_widget_show_all(spin_box);
    gtk_main_quit();
    gtk_widget_destroy(spin_box);
    gtk_container_add(GTK_CONTAINER(main_overlay), main_box);
    gtk_widget_show_all(window);
}

void add_item(GtkWidget* button, gpointer data)
{
    GtkTreeView* treeview = (GtkTreeView*)data;
    GtkTreeIter current, iter;
    struct Item* item;
    item = (struct Item*)malloc(sizeof(struct Item));

    GtkTreeModel* model = gtk_tree_view_get_model(treeview);
    GtkTreeViewColumn* column;

    /* Отмена сортировки */
    GtkTreeSortable* sortable = GTK_TREE_SORTABLE(model);
    gtk_tree_sortable_set_sort_column_id(sortable, -2, 1);

    /* Вставка новой строки после текущей */
    GtkTreePath* path;
    gtk_tree_view_get_cursor(treeview, &path, NULL);
    if (path) {
        gtk_tree_model_get_iter(model, &current, path);
        gtk_tree_path_free(path);
        gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter, &current);
    } else {
        gtk_list_store_insert(GTK_LIST_STORE(model), &iter, -1);
    }

    /* Установка данных для новой строки*/
    strcpy(item->word, "'Слово'");
    strcpy(item->translation, "'Перевод'");

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, item->word, 1, item->translation, -1);

    /* Перемещение выделения на созданную строку */
    path = gtk_tree_model_get_path(model, &iter);
    column = gtk_tree_view_get_column(treeview, 0);
    gtk_tree_view_set_cursor(treeview, path, column, FALSE);

    gtk_tree_path_free(path);
}

void remove_item(GtkWidget* widget, gpointer data)
{
    GtkTreeView* treeview = (GtkTreeView*)data;
    GtkTreeIter iter;

    GtkTreeModel* model = gtk_tree_view_get_model(treeview);
    GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);

    if (gtk_tree_selection_get_selected(selection, NULL, &iter))
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

void insert_text(GtkEntry* entry, const gchar* text, gint len, gint* position, gpointer data)
{
    GtkEditable* editable = GTK_EDITABLE(entry);
    int i, count = 0;
    gchar* result = g_new(gchar, len);

    for (i = 0; i < len; i++) {
        if (check_valid_char(*position, text[i]))
            continue;
        result[count] = text[i];
        count++;
    }

    g_signal_handlers_block_by_func(G_OBJECT(editable), G_CALLBACK(insert_text), data);
    gtk_editable_insert_text(editable, result, count, position);
    g_signal_handlers_unblock_by_func(G_OBJECT(editable), G_CALLBACK(insert_text), data);
    g_signal_stop_emission_by_name(G_OBJECT(editable), "insert_text");
    g_free(result);
}

void cell_edit(GtkCellRenderer* renderer, GtkCellEditable* editable, gchar* path, gpointer user_data)
{
    if (g_utf8_collate(gtk_entry_get_text((GtkEntry*)editable), "'Слово'") == 0 || g_utf8_collate(gtk_entry_get_text((GtkEntry*)editable), "'Перевод'") == 0)
        gtk_entry_set_text((GtkEntry*)editable, "");
    gtk_entry_set_max_length((GtkEntry*)editable, 99); // ограничение ввода
    g_signal_connect(G_OBJECT(editable), "insert_text", G_CALLBACK(insert_text), NULL);
}
void cell_edited(GtkCellRendererText* cell, const gchar* path_string, const gchar* new_text, gpointer data)
{
    struct Item* item;
    item = (struct Item*)malloc(sizeof(struct Item));
    GtkTreeModel* model = (GtkTreeModel*)data;

    /* Отмена сортировки */
    GtkTreeSortable* sortable = GTK_TREE_SORTABLE(model);
    gtk_tree_sortable_set_sort_column_id(sortable, -2, 1);

    GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);

    gint column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));

    switch (column) {
    case 0: {
        if (new_text[0] == '\0')
            strcpy(item->word, "'Слово'");
        else
            strcpy(item->word, new_text);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, item->word, -1);
    } break;

    case 1: {
        if (new_text[0] == '\0')
            strcpy(item->translation, "'Перевод'");
        else
            strcpy(item->translation, new_text);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, item->translation, -1);
    } break;
    }
}

gboolean key_press_event_voc(GtkWidget* view, GdkEventKey* event)
{
    // g_print("%d + %d \n", event->state, event->keyval);
    GtkWidget* treeview;
    treeview = (GtkWidget*)gtk_bin_get_child(GTK_BIN((GtkWidget*)gtk_container_get_children(GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(view))))))->next->data));
    if ((event->state & gtk_accelerator_get_default_mod_mask()) == GDK_CONTROL_MASK) {
        if ((event->keyval == GDK_KEY_s) || (event->keyval == GDK_KEY_S) || (event->keyval == GDK_KEY_Cyrillic_YERU) || (event->keyval == GDK_KEY_Cyrillic_yeru)) {
            write_to_bfile(gtk_bin_get_child(GTK_BIN(view)), treeview);
        } else if (event->keyval == GDK_KEY_Delete) {
            remove_item(NULL, treeview);
        } else if ((event->keyval == GDK_KEY_d) || (event->keyval == GDK_KEY_D) || (event->keyval == GDK_KEY_Cyrillic_VE) || (event->keyval == GDK_KEY_Cyrillic_ve)) {
            add_item(NULL, treeview);
        }
    } else {
        if (event->keyval == GDK_KEY_Escape) {
            g_signal_handlers_disconnect_by_func(G_OBJECT(view), key_press_event_voc, NULL);
            gtk_main_quit();
        }
    }
    return FALSE;
}

void vocabulary_win(GtkWidget* widget, gpointer data)
{
    GtkWidget* window = (GtkWidget*)data;

    g_signal_connect(window, "key-press-event", G_CALLBACK(key_press_event_voc), NULL);
    GtkWidget* main_overlay = g_object_ref((GtkWidget*)gtk_bin_get_child(GTK_BIN(window)));
    GtkWidget* main_box = g_object_ref((GtkWidget*)gtk_bin_get_child(GTK_BIN(main_overlay)));
    gtk_container_remove(GTK_CONTAINER(main_overlay), main_box);

    int info_bar_showed = 1;
    gtk_container_forall(GTK_CONTAINER(main_overlay), detect_info_bar, (gpointer)&info_bar_showed);

    GtkWidget* voc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_overlay), voc_box);

    GtkWidget *label, *btns_box, *btn_add_rec, *btn_rem_rec, *btn_back, *btn_save, *treeview, *sw;

    label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(voc_box), label, FALSE, FALSE, 30);
    gtk_widget_set_name(label, "header");
    gtk_label_set_markup(GTK_LABEL(label), "Словарь");

    btns_box = gtk_grid_new();
    gtk_widget_set_name(btns_box, "btn_voc");
    gtk_box_pack_end(GTK_BOX(voc_box), btns_box, FALSE, FALSE, 0);
    gtk_grid_set_row_homogeneous((GtkGrid*)btns_box, TRUE);
    gtk_grid_set_column_homogeneous((GtkGrid*)btns_box, TRUE);
    gtk_grid_set_row_spacing((GtkGrid*)btns_box, 10);
    gtk_grid_set_column_spacing((GtkGrid*)btns_box, 10);

    btn_rem_rec = gtk_button_new_with_label("Удалить запись");
    btn_add_rec = gtk_button_new_with_label("Добавить запись");
    btn_back = gtk_button_new_with_label("Назад");
    btn_save = gtk_button_new_with_label("Сохранить");

    gtk_grid_attach((GtkGrid*)btns_box, btn_rem_rec, 0, 0, 1, 1);
    gtk_grid_attach((GtkGrid*)btns_box, btn_add_rec, 1, 0, 1, 1);
    gtk_grid_attach((GtkGrid*)btns_box, btn_back, 0, 1, 1, 1);
    gtk_grid_attach((GtkGrid*)btns_box, btn_save, 1, 1, 1, 1);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(sw), 15);
    gtk_box_pack_start(GTK_BOX(voc_box), sw, TRUE, TRUE, 0);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    treeview = gtk_tree_view_new();
    /* Задание модели таблицы */
    GtkListStore* model;
    model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_tree_view_set_enable_search((GtkTreeView*)treeview, FALSE);
    gtk_container_add(GTK_CONTAINER(sw), treeview);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_SINGLE);

    char* names_columns[2] = {"Слово", "Перевод"};
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;

    for (int i = 0; i < 2; i++) {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(renderer, "editable", TRUE, NULL); // Редактирование строки
        g_signal_connect(renderer, "edited", G_CALLBACK(cell_edited), GTK_TREE_MODEL(model));
        g_signal_connect(renderer, "editing-started", G_CALLBACK(cell_edit), GTK_TREE_MODEL(model));
        g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(i));
        column = gtk_tree_view_column_new_with_attributes(names_columns[i], renderer, "text", i, NULL);
        gtk_tree_view_column_set_sort_column_id(column, i);
        gtk_tree_view_column_set_fixed_width(column, 1);
        gtk_tree_view_column_set_alignment(column, 0.5); // Выравнивание по центру
        gtk_tree_view_column_set_expand(column, TRUE);   // Равное разбиение между столбцами
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    }

    /* Нажатия на кнопки */
    g_signal_connect(G_OBJECT(btn_rem_rec), "clicked", G_CALLBACK(remove_item), treeview);
    g_signal_connect(G_OBJECT(btn_add_rec), "clicked", G_CALLBACK(add_item), treeview);
    g_signal_connect(G_OBJECT(btn_back), "clicked", gtk_main_quit, NULL);
    g_signal_connect(G_OBJECT(btn_save), "clicked", G_CALLBACK(write_to_bfile), treeview);

    /* Спиннер */
    GtkWidget* spinner = gtk_spinner_new();
    gtk_box_pack_end(GTK_BOX(voc_box), spinner, TRUE, TRUE, 30);

    gtk_widget_show(spinner);
    gtk_widget_show(voc_box);
    gtk_spinner_start(GTK_SPINNER(spinner));

    read_from_bfile(model);

    gtk_widget_destroy(spinner);
    gtk_widget_show_all(voc_box);
    gtk_main();
    g_signal_handlers_disconnect_by_func(window, key_press_event_voc, NULL);
    gtk_widget_destroy(voc_box);
    gtk_container_add(GTK_CONTAINER(main_overlay), main_box);
    gtk_widget_show_all(window);
}