#define MAX_DIM_ID_DISH 3
#define DIM_ROW_ORDER 9

#define UPDATE_STATUS_ORD_1 1
#define UPDATE_STATUS_ORD_2 3

#define PENDING 0
#define PREPARING 1
#define READY 2

struct Dish
{
    char dish_ID[MAX_DIM_ID_DISH];
    int quantity;
    int price;
};

struct Receipt
{
    int num_orders;
    int subtotals[MENU_SIZE];
    int total;
};

struct Order
{
    int order_ID;
    int table_ID;
    int order_counter;
    int quantity[MENU_SIZE];
    int status;
};