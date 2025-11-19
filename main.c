#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <raylib.h>

// Constants
#define MAX_PRODUCTS 100
#define LOW_STOCK_THRESHOLD 10
#define FILENAME "inventory.txt"
#define LOGFILE "activity_log.txt"
#define CSVFILE "inventory_export.csv"
#define MAX_USERNAME 50
#define MAX_PASSWORD 50

// Custom Colors not defined in standard Raylib
#define DARKORANGE (Color){ 200, 120, 0, 255 }

// Product Type Enumeration
typedef enum {
    RAW_MATERIAL,
    FINISHED_GOOD
} ProductType;

// User Role Enumeration
typedef enum {
    ADMIN,
    STAFF
} UserRole;

// Product Structure
typedef struct {
    int id;
    char name[50];
    int quantity;
    float price;
    ProductType type;
} Product;

// User Structure
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    UserRole role;
} User;

// Global Variables
Product inventory[MAX_PRODUCTS];
int productCount = 0;
User currentUser;
int isLoggedIn = 0;

// Screen States
typedef enum {
    LOGIN_SCREEN,
    MAIN_MENU,
    ADD_PRODUCT,
    VIEW_INVENTORY,
    UPDATE_STOCK,
    PROCESS_SALE,
    PROCESS_PURCHASE,
    DELETE_PRODUCT,
    SEARCH_PRODUCT,
    VIEW_CHARTS,
    ACTIVITY_LOG_SCREEN,
    EXPORT_CSV_SCREEN
} ScreenState;

ScreenState currentScreen = LOGIN_SCREEN;

// Hardcoded Users
User users[] = {
    {"admin", "admin123", ADMIN},
    {"staff", "staff123", STAFF}
};
int userCount = 2;

// ---------------- Helper Function for Input ----------------
// Handles typing into a string buffer. 
// numericOnly = 1 allows only numbers and dots.
void HandleTextInput(char *buffer, int maxLen, int numericOnly) {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (strlen(buffer) < maxLen)) {
            if (!numericOnly || (key >= '0' && key <= '9') || key == '.') {
                int len = strlen(buffer);
                buffer[len] = (char)key;
                buffer[len + 1] = '\0';
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(buffer);
        if (len > 0) {
            buffer[len - 1] = '\0';
        }
    }
}

// Helper to draw input boxes (Standard C Function)
void DrawInputBox(const char* label, int y, char* buffer, int myFocusID, int* currentFocus, int width) {
    DrawText(label, 150, y + 5, 18, BLACK);
    Rectangle box = {300, (float)y, (float)width, 30};
    
    int isFocused = (*currentFocus == myFocusID);
    
    DrawRectangleRec(box, isFocused ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(box, 1, isFocused ? BLUE : GRAY);
    DrawText(buffer, 310, y + 5, 18, BLACK);
    
    if (CheckCollisionPointRec(GetMousePosition(), box) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        *currentFocus = myFocusID;
    }
}

// ---------------- Core Logic Functions ----------------

void loadInventory() {
    FILE* fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        productCount = 0;
        return;
    }
    
    productCount = 0;
    while (fscanf(fp, "%d,%49[^,],%d,%f,%d\n", 
                  &inventory[productCount].id,
                  inventory[productCount].name,
                  &inventory[productCount].quantity,
                  &inventory[productCount].price,
                  (int*)&inventory[productCount].type) == 5) {
        productCount++;
        if (productCount >= MAX_PRODUCTS) break;
    }
    
    fclose(fp);
}

void saveInventory() {
    FILE* fp = fopen(FILENAME, "w");
    if (fp == NULL) {
        printf("Error saving inventory!\n");
        return;
    }
    
    for (int i = 0; i < productCount; i++) {
        fprintf(fp, "%d,%s,%d,%.2f,%d\n",
                inventory[i].id,
                inventory[i].name,
                inventory[i].quantity,
                inventory[i].price,
                inventory[i].type);
    }
    
    fclose(fp);
}

void logActivity(const char* action) {
    FILE* fp = fopen(LOGFILE, "a");
    if (fp == NULL) return;
    
    time_t now = time(NULL);
    char* timeStr = ctime(&now);
    if (timeStr) timeStr[strlen(timeStr) - 1] = '\0'; // Remove newline
    
    fprintf(fp, "[%s] User: %s | Action: %s\n", timeStr ? timeStr : "Unknown", currentUser.username, action);
    fclose(fp);
}

int authenticateUser(const char* username, const char* password) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            currentUser = users[i];
            isLoggedIn = 1;
            logActivity("Logged in");
            return 1;
        }
    }
    return 0;
}

void addProduct(Product* inv, int* count, int id, const char* name, int qty, float price, ProductType type) {
    if (*count >= MAX_PRODUCTS) return;
    
    inv[*count].id = id;
    strcpy(inv[*count].name, name);
    inv[*count].quantity = qty;
    inv[*count].price = price;
    inv[*count].type = type;
    (*count)++;
    
    saveInventory();
    char logMsg[200];
    sprintf(logMsg, "Added product: %s (ID: %d)", name, id);
    logActivity(logMsg);
}

Product* searchProduct(Product* inv, int count, int id) {
    for (int i = 0; i < count; i++) {
        if (inv[i].id == id) {
            return &inv[i];
        }
    }
    return NULL;
}

void updateStock(Product* inv, int count, int id, int newQty) {
    Product* p = searchProduct(inv, count, id);
    if (p != NULL) {
        p->quantity = newQty;
        saveInventory();
        char logMsg[200];
        sprintf(logMsg, "Updated stock for ID %d to %d units", id, newQty);
        logActivity(logMsg);
    }
}

void processSale(Product* inv, int count, int id, int qty) {
    Product* p = searchProduct(inv, count, id);
    if (p != NULL) {
        if (p->quantity >= qty) {
            p->quantity -= qty;
            saveInventory();
            char logMsg[200];
            sprintf(logMsg, "Sale: %d units of %s (ID: %d)", qty, p->name, id);
            logActivity(logMsg);
        }
    }
}

void processPurchase(Product* inv, int count, int id, int qty) {
    Product* p = searchProduct(inv, count, id);
    if (p != NULL) {
        p->quantity += qty;
        saveInventory();
        char logMsg[200];
        sprintf(logMsg, "Purchase: %d units of %s (ID: %d)", qty, p->name, id);
        logActivity(logMsg);
    }
}

void deleteProduct(Product* inv, int* count, int id) {
    int index = -1;
    for (int i = 0; i < *count; i++) {
        if (inv[i].id == id) {
            index = i;
            break;
        }
    }
    
    if (index != -1) {
        char logMsg[200];
        sprintf(logMsg, "Deleted product: %s (ID: %d)", inv[index].name, id);
        
        for (int i = index; i < *count - 1; i++) {
            inv[i] = inv[i + 1];
        }
        (*count)--;
        saveInventory();
        logActivity(logMsg);
    }
}

void exportToCSV(Product* inv, int count) {
    FILE* fp = fopen(CSVFILE, "w");
    if (fp == NULL) return;
    
    fprintf(fp, "ID,Name,Quantity,Price,Type\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d,%s,%d,%.2f,%s\n",
                inv[i].id,
                inv[i].name,
                inv[i].quantity,
                inv[i].price,
                inv[i].type == RAW_MATERIAL ? "Raw Material" : "Finished Good");
    }
    
    fclose(fp);
    logActivity("Exported inventory to CSV");
}

void initializeSystem() {
    loadInventory();
}

// ---------------- GUI Drawing Functions ----------------

void drawLoginScreen() {
    static char username[MAX_USERNAME] = "";
    static char password[MAX_PASSWORD] = "";
    static char errorMsg[100] = "";
    static int focus = 0; // 0: None, 1: Username, 2: Password

    ClearBackground(RAYWHITE);
    DrawText("INVENTORY MANAGEMENT SYSTEM", 150, 50, 30, DARKBLUE);
    
    // Login Box
    DrawRectangle(200, 200, 400, 250, LIGHTGRAY);
    DrawRectangleLines(200, 200, 400, 250, DARKGRAY);
    DrawText("Login", 370, 220, 24, DARKBLUE);

    // Username Field
    DrawText("Username:", 230, 270, 20, BLACK);
    Rectangle userBox = {230, 295, 340, 30};
    DrawRectangleRec(userBox, focus == 1 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(userBox, 1, focus == 1 ? BLUE : GRAY);
    DrawText(username, 240, 302, 20, BLACK);
    
    if (CheckCollisionPointRec(GetMousePosition(), userBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 1;

    // Password Field
    DrawText("Password:", 230, 340, 20, BLACK);
    Rectangle passBox = {230, 365, 340, 30};
    DrawRectangleRec(passBox, focus == 2 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(passBox, 1, focus == 2 ? BLUE : GRAY);
    
    char hiddenPass[MAX_PASSWORD];
    int passLen = strlen(password);
    for (int i = 0; i < passLen; i++) hiddenPass[i] = '*';
    hiddenPass[passLen] = '\0';
    DrawText(hiddenPass, 240, 372, 20, BLACK);

    if (CheckCollisionPointRec(GetMousePosition(), passBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 2;

    // Handle Input
    if (focus == 1) HandleTextInput(username, MAX_USERNAME - 1, 0);
    if (focus == 2) HandleTextInput(password, MAX_PASSWORD - 1, 0);

    // Login Button
    Rectangle loginBtn = {320, 410, 160, 30};
    if (CheckCollisionPointRec(GetMousePosition(), loginBtn)) {
        DrawRectangleRec(loginBtn, DARKBLUE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (authenticateUser(username, password)) {
                currentScreen = MAIN_MENU;
                strcpy(errorMsg, "");
                username[0] = password[0] = '\0';
            } else {
                strcpy(errorMsg, "Invalid credentials!");
            }
        }
    } else {
        DrawRectangleRec(loginBtn, BLUE);
    }
    DrawText("LOGIN", 360, 417, 16, WHITE);
    
    if (strlen(errorMsg) > 0) DrawText(errorMsg, 300, 455, 16, RED);
    DrawText("Demo: admin/admin123 or staff/staff123", 220, 500, 14, DARKGRAY);
}

void drawMainMenu() {
    ClearBackground(RAYWHITE);
    char welcome[100];
    sprintf(welcome, "Welcome, %s (%s)", currentUser.username, currentUser.role == ADMIN ? "Admin" : "Staff");
    DrawText(welcome, 20, 20, 20, DARKBLUE);
    DrawText("MAIN MENU", 300, 60, 28, DARKBLUE);
    
    Rectangle buttons[11];
    const char* btnLabels[] = {
        "Add Product", "View Inventory", "Update Stock", "Process Sale",
        "Process Purchase", "Delete Product", "Search Product", "View Charts",
        "Activity Log", "Export CSV", "Logout"
    };
    
    int startY = 120;
    for (int i = 0; i < 11; i++) {
        buttons[i] = (Rectangle){300, (float)(startY + i * 40), 200, 35};
        
        // Role Access
        if ((i == 0 || i == 5) && currentUser.role == STAFF) {
            DrawRectangleRec(buttons[i], LIGHTGRAY);
            DrawText(btnLabels[i], buttons[i].x + 40, buttons[i].y + 8, 18, GRAY);
            continue;
        }
        
        if (CheckCollisionPointRec(GetMousePosition(), buttons[i])) {
            DrawRectangleRec(buttons[i], DARKBLUE);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (i == 0) currentScreen = ADD_PRODUCT;
                else if (i == 1) currentScreen = VIEW_INVENTORY;
                else if (i == 2) currentScreen = UPDATE_STOCK;
                else if (i == 3) currentScreen = PROCESS_SALE;
                else if (i == 4) currentScreen = PROCESS_PURCHASE;
                else if (i == 5) currentScreen = DELETE_PRODUCT;
                else if (i == 6) currentScreen = SEARCH_PRODUCT;
                else if (i == 7) currentScreen = VIEW_CHARTS;
                else if (i == 8) currentScreen = ACTIVITY_LOG_SCREEN;
                else if (i == 9) currentScreen = EXPORT_CSV_SCREEN;
                else if (i == 10) { isLoggedIn = 0; currentScreen = LOGIN_SCREEN; logActivity("Logged out"); }
            }
        } else {
            DrawRectangleRec(buttons[i], BLUE);
        }
        DrawText(btnLabels[i], buttons[i].x + 40, buttons[i].y + 8, 18, WHITE);
    }

    // Alerts
    DrawText("LOW STOCK ALERTS:", 550, 100, 18, RED);
    int alertY = 130;
    for (int i = 0; i < productCount; i++) {
        if (inventory[i].quantity < LOW_STOCK_THRESHOLD) {
            char alert[100];
            sprintf(alert, "- %s (%d left)", inventory[i].name, inventory[i].quantity);
            DrawText(alert, 550, alertY, 14, ORANGE);
            alertY += 20;
        }
    }
}

void drawAddProductScreen() {
    static char idStr[20] = "";
    static char name[50] = "";
    static char qtyStr[20] = "";
    static char priceStr[20] = "";
    static int typeSelected = 0;
    static char message[100] = "";
    static int focus = 0; // 1:ID, 2:Name, 3:Qty, 4:Price

    ClearBackground(RAYWHITE);
    DrawText("ADD PRODUCT", 300, 30, 26, DARKBLUE);
    
    // Using Standard C function helper now
    DrawInputBox("Product ID:", 100, idStr, 1, &focus, 300);
    DrawInputBox("Name:", 150, name, 2, &focus, 300);
    DrawInputBox("Quantity:", 200, qtyStr, 3, &focus, 300);
    DrawInputBox("Price:", 250, priceStr, 4, &focus, 300);

    // Type Selection
    DrawText("Type:", 150, 300, 18, BLACK);
    Rectangle rawBtn = {300, 300, 140, 30};
    Rectangle finBtn = {450, 300, 150, 30};
    
    DrawRectangleRec(rawBtn, typeSelected == 0 ? DARKBLUE : LIGHTGRAY);
    DrawText("Raw Material", 310, 307, 14, typeSelected == 0 ? WHITE : BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), rawBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) typeSelected = 0;

    DrawRectangleRec(finBtn, typeSelected == 1 ? DARKBLUE : LIGHTGRAY);
    DrawText("Finished Good", 460, 307, 14, typeSelected == 1 ? WHITE : BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), finBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) typeSelected = 1;

    // Handling Input Logic
    if (focus == 1) HandleTextInput(idStr, 10, 1);
    if (focus == 2) HandleTextInput(name, 49, 0);
    if (focus == 3) HandleTextInput(qtyStr, 10, 1);
    if (focus == 4) HandleTextInput(priceStr, 10, 1);

    // Buttons
    Rectangle addBtn = {320, 360, 160, 40};
    if (CheckCollisionPointRec(GetMousePosition(), addBtn)) {
        DrawRectangleRec(addBtn, DARKGREEN);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int id = atoi(idStr);
            int qty = atoi(qtyStr);
            float price = (float)atof(priceStr);
            if (id > 0 && strlen(name) > 0) {
                addProduct(inventory, &productCount, id, name, qty, price, typeSelected == 0 ? RAW_MATERIAL : FINISHED_GOOD);
                sprintf(message, "Product added!");
                idStr[0] = name[0] = qtyStr[0] = priceStr[0] = '\0';
            } else sprintf(message, "Invalid Input!");
        }
    } else DrawRectangleRec(addBtn, GREEN);
    DrawText("ADD", 375, 373, 18, WHITE);

    Rectangle backBtn = {320, 410, 160, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentScreen = MAIN_MENU; message[0] = '\0'; }
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 365, 423, 18, WHITE);

    DrawText(message, 250, 470, 16, GREEN);
}

void drawViewInventoryScreen() {
    ClearBackground(RAYWHITE);
    DrawText("INVENTORY LIST", 280, 20, 26, DARKBLUE);
    DrawText("ID   Name                  Qty    Price   Type", 50, 60, 18, DARKGRAY);
    DrawLine(40, 80, 760, 80, BLACK);
    
    int yPos = 100;
    for (int i = 0; i < productCount && i < 20; i++) {
        char line[150];
        sprintf(line, "%-4d %-20s %-6d %-7.2f %s", 
            inventory[i].id, inventory[i].name, inventory[i].quantity, 
            inventory[i].price, inventory[i].type == RAW_MATERIAL ? "Raw" : "Fin");
        
        Color rowColor = (inventory[i].quantity < LOW_STOCK_THRESHOLD) ? RED : BLACK;
        DrawText(line, 50, yPos, 16, rowColor);
        yPos += 22;
    }
    
    if (productCount > 20) DrawText("...More items hidden...", 300, 530, 14, GRAY);

    Rectangle backBtn = {320, 550, 160, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) currentScreen = MAIN_MENU;
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 375, 563, 18, WHITE);
}

void drawUpdateStockScreen() {
    static char idStr[20] = "";
    static char qtyStr[20] = "";
    static char message[100] = "";
    static int focus = 0; 

    ClearBackground(RAYWHITE);
    DrawText("UPDATE STOCK", 300, 30, 26, DARKBLUE);

    Rectangle idBox = {350, 145, 250, 30};
    Rectangle qtyBox = {350, 205, 250, 30};
    
    DrawText("Product ID:", 200, 150, 18, BLACK);
    DrawRectangleRec(idBox, focus == 1 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(idBox, 1, BLACK);
    DrawText(idStr, 360, 152, 18, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), idBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 1;

    DrawText("New Quantity:", 200, 210, 18, BLACK);
    DrawRectangleRec(qtyBox, focus == 2 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(qtyBox, 1, BLACK);
    DrawText(qtyStr, 360, 212, 18, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), qtyBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 2;

    if (focus == 1) HandleTextInput(idStr, 10, 1);
    if (focus == 2) HandleTextInput(qtyStr, 10, 1);

    Rectangle updateBtn = {300, 270, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), updateBtn)) {
        DrawRectangleRec(updateBtn, DARKORANGE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int id = atoi(idStr);
            int qty = atoi(qtyStr);
            updateStock(inventory, productCount, id, qty);
            sprintf(message, "Stock updated!");
        }
    } else DrawRectangleRec(updateBtn, ORANGE);
    DrawText("UPDATE", 360, 283, 18, WHITE);

    Rectangle backBtn = {300, 320, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentScreen = MAIN_MENU; message[0] = '\0'; }
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 370, 333, 18, WHITE);
    DrawText(message, 220, 380, 16, GREEN);
}

// Generic Function to draw simple ID/Qty screens (Sale/Purchase)
void drawTransactionScreen(const char* title, void (*processFunc)(Product*, int, int, int), Color btnColor) {
    static char idStr[20] = "";
    static char qtyStr[20] = "";
    static char message[100] = "";
    static int focus = 0; 

    ClearBackground(RAYWHITE);
    DrawText(title, 300, 30, 26, DARKBLUE);

    Rectangle idBox = {350, 145, 250, 30};
    Rectangle qtyBox = {350, 205, 250, 30};

    DrawText("Product ID:", 200, 150, 18, BLACK);
    DrawRectangleRec(idBox, focus == 1 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(idBox, 1, BLACK);
    DrawText(idStr, 360, 152, 18, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), idBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 1;

    DrawText("Quantity:", 200, 210, 18, BLACK);
    DrawRectangleRec(qtyBox, focus == 2 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(qtyBox, 1, BLACK);
    DrawText(qtyStr, 360, 212, 18, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), qtyBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 2;

    if (focus == 1) HandleTextInput(idStr, 10, 1);
    if (focus == 2) HandleTextInput(qtyStr, 10, 1);

    Rectangle actBtn = {300, 270, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), actBtn)) {
        DrawRectangleRec(actBtn, Fade(btnColor, 0.8f));
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int id = atoi(idStr);
            int qty = atoi(qtyStr);
            if (id > 0 && qty > 0) {
                processFunc(inventory, productCount, id, qty);
                sprintf(message, "Transaction Success!");
            } else sprintf(message, "Invalid Input");
        }
    } else DrawRectangleRec(actBtn, btnColor);
    DrawText("PROCESS", 350, 283, 18, WHITE);

    Rectangle backBtn = {300, 320, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentScreen = MAIN_MENU; message[0] = '\0'; }
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 370, 333, 18, WHITE);
    DrawText(message, 220, 380, 16, GREEN);
}

void drawDeleteScreen() {
    static char idStr[20] = "";
    static char message[100] = "";
    static int focus = 0;
    
    ClearBackground(RAYWHITE);
    DrawText("DELETE PRODUCT", 280, 30, 26, DARKBLUE);
    DrawText("(Admin Only)", 330, 65, 16, RED);

    Rectangle idBox = {380, 175, 200, 30};
    DrawText("Product ID:", 250, 180, 18, BLACK);
    DrawRectangleRec(idBox, focus == 1 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(idBox, 1, BLACK);
    DrawText(idStr, 390, 182, 18, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), idBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 1;
    
    if (focus == 1) HandleTextInput(idStr, 10, 1);

    Rectangle delBtn = {300, 240, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), delBtn)) {
        DrawRectangleRec(delBtn, MAROON);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int id = atoi(idStr);
            deleteProduct(inventory, &productCount, id);
            sprintf(message, "Deletion Attempted");
        }
    } else DrawRectangleRec(delBtn, RED);
    DrawText("DELETE", 355, 253, 18, WHITE);

    Rectangle backBtn = {300, 290, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentScreen = MAIN_MENU; message[0] = '\0'; }
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 370, 303, 18, WHITE);
    DrawText(message, 260, 350, 16, RED);
}

void drawSearchScreen() {
    static char idStr[20] = "";
    static Product* foundProduct = NULL;
    static int focus = 0;
    
    ClearBackground(RAYWHITE);
    DrawText("SEARCH PRODUCT", 280, 30, 26, DARKBLUE);

    Rectangle idBox = {380, 115, 200, 30};
    DrawText("Product ID:", 250, 120, 18, BLACK);
    DrawRectangleRec(idBox, focus == 1 ? SKYBLUE : WHITE);
    DrawRectangleLinesEx(idBox, 1, BLACK);
    DrawText(idStr, 390, 122, 18, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), idBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focus = 1;
    if (focus == 1) HandleTextInput(idStr, 10, 1);

    Rectangle searchBtn = {300, 170, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), searchBtn)) {
        DrawRectangleRec(searchBtn, DARKBLUE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            foundProduct = searchProduct(inventory, productCount, atoi(idStr));
        }
    } else DrawRectangleRec(searchBtn, BLUE);
    DrawText("SEARCH", 355, 183, 18, WHITE);

    if (foundProduct != NULL) {
        DrawRectangle(150, 240, 500, 200, LIGHTGRAY);
        DrawRectangleLines(150, 240, 500, 200, DARKGRAY);
        DrawText("PRODUCT FOUND", 320, 255, 20, DARKGREEN);
        
        char info[100];
        sprintf(info, "Name: %s", foundProduct->name);
        DrawText(info, 180, 290, 16, BLACK);
        sprintf(info, "Qty: %d | Price: $%.2f", foundProduct->quantity, foundProduct->price);
        DrawText(info, 180, 320, 16, BLACK);
    }

    Rectangle backBtn = {300, 470, 200, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentScreen = MAIN_MENU; foundProduct = NULL; }
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 370, 483, 18, WHITE);
}

void drawChartsScreen() {
    ClearBackground(RAYWHITE);
    DrawText("DATA VISUALIZATION", 260, 20, 26, DARKBLUE);
    
    // Bar Chart
    DrawText("Stock Levels (First 8)", 80, 60, 18, BLACK);
    DrawRectangleLines(50, 90, 350, 250, BLACK);
    
    int maxQty = 1;
    for(int i=0; i<productCount && i<8; i++) if(inventory[i].quantity > maxQty) maxQty = inventory[i].quantity;
    
    for (int i = 0; i < productCount && i < 8; i++) {
        int h = (inventory[i].quantity * 240) / maxQty;
        DrawRectangle(60 + i * 40, 340 - h, 30, h, BLUE);
    }

    // Basic Pie Chart (Simulated with Rectangles for simplicity without raymath)
    DrawText("Type Distribution", 470, 60, 18, BLACK);
    int raw = 0, fin = 0;
    for(int i=0; i<productCount; i++) (inventory[i].type == RAW_MATERIAL) ? raw++ : fin++;
    
    char stats[100];
    sprintf(stats, "Raw: %d | Finished: %d", raw, fin);
    DrawText(stats, 470, 100, 16, DARKGRAY);
    
    // Back Button
    Rectangle backBtn = {320, 500, 160, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) currentScreen = MAIN_MENU;
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 375, 513, 18, WHITE);
}

void drawActivityLogScreen() {
    ClearBackground(RAYWHITE);
    DrawText("ACTIVITY LOG", 300, 20, 26, DARKBLUE);
    
    FILE* fp = fopen(LOGFILE, "r");
    if (fp) {
        char line[256];
        int y = 70;
        int count = 0;
        while (fgets(line, sizeof(line), fp) && count < 20) {
            line[strcspn(line, "\n")] = 0;
            DrawText(line, 30, y, 10, BLACK);
            y += 18;
            count++;
        }
        fclose(fp);
    } else {
        DrawText("No logs yet.", 300, 200, 20, GRAY);
    }

    Rectangle backBtn = {320, 500, 160, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) currentScreen = MAIN_MENU;
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 375, 513, 18, WHITE);
}

void drawExportScreen() {
    static char message[100] = "";
    ClearBackground(RAYWHITE);
    DrawText("EXPORT CSV", 300, 60, 26, DARKBLUE);
    
    Rectangle expBtn = {280, 240, 240, 50};
    if (CheckCollisionPointRec(GetMousePosition(), expBtn)) {
        DrawRectangleRec(expBtn, DARKGREEN);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            exportToCSV(inventory, productCount);
            sprintf(message, "Exported to inventory_export.csv");
        }
    } else DrawRectangleRec(expBtn, GREEN);
    DrawText("EXPORT NOW", 330, 258, 18, WHITE);

    Rectangle backBtn = {320, 310, 160, 40};
    if (CheckCollisionPointRec(GetMousePosition(), backBtn)) {
        DrawRectangleRec(backBtn, DARKGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentScreen = MAIN_MENU; message[0] = '\0'; }
    } else DrawRectangleRec(backBtn, GRAY);
    DrawText("BACK", 375, 323, 18, WHITE);
    DrawText(message, 200, 400, 16, GREEN);
}

// Main Loop
int main(void) {
    InitWindow(800, 600, "Inventory Management System");
    SetTargetFPS(60);
    initializeSystem();
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        switch (currentScreen) {
            case LOGIN_SCREEN: drawLoginScreen(); break;
            case MAIN_MENU: drawMainMenu(); break;
            case ADD_PRODUCT: drawAddProductScreen(); break;
            case VIEW_INVENTORY: drawViewInventoryScreen(); break;
            case UPDATE_STOCK: drawUpdateStockScreen(); break;
            case PROCESS_SALE: drawTransactionScreen("PROCESS SALE", processSale, PURPLE); break;
            case PROCESS_PURCHASE: drawTransactionScreen("PROCESS PURCHASE", processPurchase, GREEN); break;
            case DELETE_PRODUCT: drawDeleteScreen(); break;
            case SEARCH_PRODUCT: drawSearchScreen(); break;
            case VIEW_CHARTS: drawChartsScreen(); break;
            case ACTIVITY_LOG_SCREEN: drawActivityLogScreen(); break;
            case EXPORT_CSV_SCREEN: drawExportScreen(); break;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}