# Computer Networks project

This project was realized for the Computer Networks course at University of Pisa in 2022/2023.

It is a *C application based on a client-server paradigm*.
Conscious of all its limitations, I based the system core functionalities in a *centralized server* rather than a peer-to-peer application.
The main reason behind this choice is that the application is intended to be a prototype of a more complex one.
It *wasn't required to be scalable* and there was no previous analysis of the amount of clients that would log
into the system and, consequently, of the amount of traffic directed to the server.
By looking and executing the code located in kd.c and td.c, I noticed that this is the real bottleneck of the
whole application.
The server acts like an intermediary between all other subjects interacting within the system: it intercepts booking requests from clients, it forwards orders and errors between the kitchen and each table issuing orders in the restaurant room.
There are some valid aspects that implicitly *authenticate the client* server side:
- each table in the restaurant room can use one tablet for communicating with the server;
- each tablet has a unique preinstantiated socket;
- each booking is univoquely identified by a *numeric code* that the server retrieves at the beginning of each workshift and that must corresponds to the one forwarded by the client during the *authentication phase*;

## Project Structure
.
|
|- client_side
    |- communication_utilities
        |- soc_client_utils.c
    |- headers
        |- header_client.h
        |- header_kitchen.h
        |- header_order.h
        |- header_table_device.h
    |- order_shared_utils.c
|- server_side
    |- server_exclusive
        |- header_server.h
        |- server_utilities.c
        |- server.c
    |- order_shared_utils.c
|- shared
    |- header.h
    |- shared_utils.c
    |- soc_utils_shared.c
|- kd.c
|- cli.c
|- td.c
|- README.md