#include <gtk/gtk.h>

static void print_hello(GtkWidget *widget, gpointer   data) {
  g_print("Hello World\n");
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window = gtk_application_window_new(app);
  /* GTK_WINDOW(window) is the same as the standard C cast operator 
    "(GtkWindow*) window" but does some extra checking */
  gtk_window_set_title(GTK_WINDOW(window), "Hekjllo");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

  GtkWidget *button = gtk_button_new_with_label("Hello World");  
  gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
  g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
  gtk_window_set_child(GTK_WINDOW(window), button);

  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new(
    "org.gtk.example", 
    G_APPLICATION_DEFAULT_FLAGS
  );

  /* Make the activate function listen for the "activate" signal. The signal is 
    sent when the application is launched and that will trigger the function */ 
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

  // Status code is received when the application is closed
  int status = g_application_run(G_APPLICATION(app), argc, argv);

  // Free the GTK Application from memory
  g_object_unref(app);

  return status;
}