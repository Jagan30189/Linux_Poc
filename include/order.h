#ifndef ORDER_H
#define ORDER_H

#define CUSTOMER_NAME_LEN 50

typedef struct
{
    int order_id;
    char customer_name[CUSTOMER_NAME_LEN];
    int product_id;
    int quantity;
    double amount;
    int priority;

} Order;

#endif
