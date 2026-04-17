# include "my_logger.h"

int main() {
    // Example usage
    const char *filename = "app.log";
    int a = 5;
    log_message(filename, "Application started");
    log_message(filename, "Processing data... %d",a);
    log_message(filename, "Operation completed successfully");
    
    return 0;
}