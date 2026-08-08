#include "museum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_dashboard_json(void)
{
    int exhibits;
    int visitors;
    int tickets;
    double revenue;

    if (!get_dashboard(&exhibits,
                       &visitors,
                       &tickets,
                       &revenue)) {
        printf("{\"error\":\"Unable to load dashboard data\"}\n");
        return;
    }

    printf(
        "{\"exhibits\":%d,"
        "\"visitors\":%d,"
        "\"tickets\":%d,"
        "\"revenue\":%.2f}\n",
        exhibits,
        visitors,
        tickets,
        revenue
    );
}

static void print_usage(void)
{
    printf("Museum Management System\n\n");

    printf("Interactive:\n");
    printf("  ./museum_core\n\n");

    printf("Data commands:\n");
    printf("  ./museum_core --dashboard\n");
    printf("  ./museum_core --list\n");
    printf("  ./museum_core --tickets\n");
    printf("  ./museum_core --popular\n");
    printf("  ./museum_core --revenue\n\n");

    printf("Management commands:\n");
    printf("  ./museum_core --add-exhibit \"Name\" \"Category\"\n");
    printf("  ./museum_core --search <id>\n");
    printf("  ./museum_core --purchase <exhibit_id> \"Visitor\" <price>\n");
    printf("  ./museum_core --delete <id>\n");
}

static int parse_id(const char *text, int *id)
{
    char *end;
    long value;

    if (!text || !id) {
        return 0;
    }

    value = strtol(text, &end, 10);

    if (end == text || *end != '\0') {
        return 0;
    }

    if (value < 1 || value > 2147483647L) {
        return 0;
    }

    *id = (int)value;

    return 1;
}

static int parse_price(const char *text, double *price)
{
    char *end;
    double value;

    if (!text || !price) {
        return 0;
    }

    value = strtod(text, &end);

    if (end == text || *end != '\0') {
        return 0;
    }

    if (value <= 0.0) {
        return 0;
    }

    *price = value;

    return 1;
}

static int handle_add_exhibit(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(
            stderr,
            "Usage: ./museum_core --add-exhibit "
            "\"Name\" \"Category\"\n"
        );

        return EXIT_FAILURE;
    }

    int id = add_exhibit(argv[2], argv[3]);

    if (id == 0) {
        fprintf(stderr, "Unable to add exhibit.\n");
        return EXIT_FAILURE;
    }

    printf(
        "{\"success\":true,\"exhibitID\":%d}\n",
        id
    );

    return EXIT_SUCCESS;
}

static int handle_search(int argc, char *argv[])
{
    int id;
    Exhibit exhibit;

    if (argc != 3 || !parse_id(argv[2], &id)) {
        fprintf(
            stderr,
            "Usage: ./museum_core --search <id>\n"
        );

        return EXIT_FAILURE;
    }

    if (!search_exhibit_by_id(id, &exhibit)) {
        printf("{\"found\":false}\n");
        return EXIT_SUCCESS;
    }

    printf(
        "{\"found\":true,"
        "\"id\":%d,"
        "\"name\":\"%s\","
        "\"category\":\"%s\","
        "\"visitors\":%d}\n",
        exhibit.exhibitID,
        exhibit.name,
        exhibit.category,
        exhibit.visitorCount
    );

    return EXIT_SUCCESS;
}

static int handle_purchase(int argc, char *argv[])
{
    int exhibit_id;
    double price;

    if (argc != 5 ||
        !parse_id(argv[2], &exhibit_id) ||
        !parse_price(argv[4], &price)) {

        fprintf(
            stderr,
            "Usage: ./museum_core "
            "--purchase <exhibit_id> "
            "\"Visitor\" <price>\n"
        );

        return EXIT_FAILURE;
    }

    int ticket_id = purchase_ticket(
        exhibit_id,
        argv[3],
        price
    );

    if (ticket_id == 0) {
        fprintf(stderr, "Unable to purchase ticket.\n");
        return EXIT_FAILURE;
    }

    printf(
        "{\"success\":true,\"ticketID\":%d}\n",
        ticket_id
    );

    return EXIT_SUCCESS;
}

static int handle_delete(int argc, char *argv[])
{
    int id;

    if (argc != 3 || !parse_id(argv[2], &id)) {
        fprintf(
            stderr,
            "Usage: ./museum_core --delete <id>\n"
        );

        return EXIT_FAILURE;
    }

    if (!delete_exhibit(id)) {
        printf("{\"success\":false}\n");
        return EXIT_SUCCESS;
    }

    printf("{\"success\":true}\n");

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        run_menu();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "--dashboard") == 0) {
        print_dashboard_json();
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "--list") == 0) {
        return list_exhibits() >= 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--tickets") == 0) {
        return list_tickets() >= 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--popular") == 0) {
        return report_popular_exhibits() >= 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--revenue") == 0) {
        return report_revenue() >= 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--add-exhibit") == 0) {
        return handle_add_exhibit(argc, argv);
    }

    if (strcmp(argv[1], "--search") == 0) {
        return handle_search(argc, argv);
    }

    if (strcmp(argv[1], "--purchase") == 0) {
        return handle_purchase(argc, argv);
    }

    if (strcmp(argv[1], "--delete") == 0) {
        return handle_delete(argc, argv);
    }

    print_usage();

    return EXIT_FAILURE;
}