#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("[Welcome to the chess game, enter commnands without having to echo and then cat; enter \"exit\" to exit]\n");
    int run = 1; 
    while (run) {
        char user_input[20];
        printf("Enter a command: ");
        fgets(user_input, sizeof(user_input), stdin);
        if (strcmp(user_input, "exit\n") == 0) {
            run = 0;
        }
        else {
            char command[100];
            snprintf(command, sizeof(command), "echo -n \"%s\" > /dev/chess", user_input);

            system(command);

            system("cat /dev/chess");
        }
    }
    printf("Ending Program\n");
    return EXIT_SUCCESS;
}
