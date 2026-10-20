#include <iostream>
#include <string>
#include <stack>
#include <cstdarg>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct UserSettings {
    string fullName       = "Samarpan";
    string email          = "samarpan@kiit.ac.in";
    string theme          = "night";
    string language       = "English (US)";
    string timezone       = "GMT+05:30";
    string priorityModel  = "Balanced";
    bool autoSchedule     = true;
    bool smartReminders   = true;
    bool emailNotif       = true;
    bool pushNotif        = false;
    bool aiSuggestions    = true;
};
UserSettings g_settings;

int getDigitFrequencyScore(const string& s) {
    int freq[10] = {0};
    for (char c : s) { if (c >= '0' && c <= '9') freq[c - '0']++; }
    int score = 0;
    for (int i = 0; i < 10; i++) score += freq[i] * (i + 1);
    return score;
}

string toLowerCase(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

class Task {
public:
    string title, description, dueDate, priority;
    int taskId, progress;
    string status, statusMsg;
    int score;
    Task(string t, string d, string date, string p)
        : title(t), description(d), dueDate(date), priority(p) {
        taskId = getDigitFrequencyScore(t) + t.length();
        progress = 0; status = "Queued";
        statusMsg = "Waiting in queue..."; score = 0;
    }
};

class Recommendation {
public:
    string type, title, description, action, meta;
    int confidence;
    Recommendation(string t, string ti, string d, string a, string m, int c)
        : type(t), title(ti), description(d), action(a), meta(m), confidence(c) {}
};

struct TaskNode { Task* task; TaskNode* next; TaskNode(Task* t) : task(t), next(nullptr) {} };
struct RecNode  { Recommendation* rec; RecNode* next; RecNode(Recommendation* r) : rec(r), next(nullptr) {} };

TaskNode* head = nullptr;
RecNode*  recHead = nullptr;
stack<Task*> recentTasks;

void addTaskToList(Task* newTask) {
    TaskNode* newNode = new TaskNode(newTask);
    if (!head) head = newNode;
    else {
        TaskNode* temp = head;
        while (temp->next) temp = temp->next;
        temp->next = newNode;
    }
    recentTasks.push(newTask);
}

void addRecommendation(Recommendation* rec) {
    RecNode* newNode = new RecNode(rec);
    if (!recHead) recHead = newNode;
    else {
        RecNode* temp = recHead;
        while (temp->next) temp = temp->next;
        temp->next = newNode;
    }
}

void clearRecommendations() {
    RecNode* temp = recHead;
    while (temp) { RecNode* nx = temp->next; delete temp->rec; delete temp; temp = nx; }
    recHead = nullptr;
}

void logTaskVariadic(int count, ...) {
    va_list args;
    va_start(args, count);
    cout << "\n[New Task Added!] Details: ";
    for (int i = 0; i < count; i++) cout << va_arg(args, const char*) << " | ";
    cout << endl;
    va_end(args);
}

int daysUntilDue(const string& dateStr) {
    if (dateStr.empty() || dateStr == "No Deadline") return 365;
    int y, m, d;
    if (sscanf(dateStr.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return 365;
    time_t now = time(0); tm* ltm = localtime(&now);
    int tY = 1900 + ltm->tm_year, tM = 1 + ltm->tm_mon, tD = ltm->tm_mday;
    return (y * 365 + m * 30 + d) - (tY * 365 + tM * 30 + tD);
}

int calculateScore(Task* task, int uW, int iW, int eW) {
    int uScore = 20;
    int days = daysUntilDue(task->dueDate);
    if (days <= 0) uScore = 100;
    else if (days == 1) uScore = 85;
    else if (days <= 3) uScore = 70;
    else if (days <= 7) uScore = 50;
    else if (days <= 30) uScore = 30;
    else uScore = 15;

    int iScore = 50;
    if (task->priority == "critical") iScore = 100;
    else if (task->priority == "high") iScore = 80;
    else if (task->priority == "low") iScore = 30;

    int eScore = getDigitFrequencyScore(task->title);
    if (eScore > 100) eScore = 100;
    if (eScore < 20) eScore = 20;
    if (task->priority == "critical") eScore = (eScore + 100) / 2;
    else if (task->priority == "high") eScore = (eScore + 80) / 2;

    return (uScore * uW + iScore * iW + eScore * eW) / 100;
}

void sortTasksByScore() {
    if (!head || !head->next) return;
    bool swapped; TaskNode* ptr1; TaskNode* lptr = nullptr;
    do {
        swapped = false; ptr1 = head;
        while (ptr1->next != lptr) {
            if (ptr1->task->score < ptr1->next->task->score) {
                Task* temp = ptr1->task;
                ptr1->task = ptr1->next->task;
                ptr1->next->task = temp;
                swapped = true;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}

void generateRecommendations() {
    clearRecommendations();
    TaskNode* temp = head;
    int taskCount = 0, criticalCount = 0, highCount = 0, noDeadlineCount = 0;
    Task* topTask = nullptr;
    int topScore = 0;

    while (temp) {
        taskCount++;
        int s = calculateScore(temp->task, 80, 65, 45);
        if (s > topScore) { topScore = s; topTask = temp->task; }
        if (temp->task->priority == "critical") criticalCount++;
        if (temp->task->priority == "high") highCount++;
        if (temp->task->dueDate == "No Deadline" || temp->task->dueDate.empty()) noDeadlineCount++;
        temp = temp->next;
    }

    if (taskCount == 0) {
        addRecommendation(new Recommendation("AI TIP", "No tasks yet",
            "Add your first task to get personalized AI recommendations based on your productivity patterns.",
            "Add Task", "Get Started", 50));
        return;
    }

    if (topTask) {
        int days = daysUntilDue(topTask->dueDate);
        string timeMsg = "due in " + to_string(days) + " days";
        if (days <= 0) timeMsg = "overdue or due today";
        else if (days == 1) timeMsg = "due tomorrow";

        addRecommendation(new Recommendation("HIGH IMPACT",
            "Start \"" + topTask->title + "\" now",
            "This task has the highest priority score (" + to_string(topScore) +
            ") and is " + timeMsg + ". Starting now reduces deadline stress.",
            "Add to schedule", "Priority: " + topTask->priority, min(95, topScore)));

        addRecommendation(new Recommendation("BREAK IT DOWN",
            "Split \"" + topTask->title + "\"",
            "Breaking large tasks into 3-4 smaller steps increases completion rate by 2x. "
            "Try: Research (30m), Draft (1h), Review (30m).",
            "Create subtasks", topTask->priority, 88));
    }

    if (criticalCount > 0) {
        addRecommendation(new Recommendation("TIME OPTIMIZATION",
            "Focus on " + to_string(criticalCount) + " critical task(s)",
            "You have " + to_string(criticalCount) +
            " critical tasks pending. Schedule these during your peak productivity hours.",
            "Block time", "Critical: " + to_string(criticalCount), 85));
    }

    if (highCount > 2) {
        addRecommendation(new Recommendation("WELLNESS",
            "Take a strategic break",
            "With " + to_string(highCount) +
            " high-priority tasks, schedule a 15-min break after every 90 minutes of work.",
            "Set reminder", "Workload: High", 75));
    } else if (highCount > 0) {
        addRecommendation(new Recommendation("TIME OPTIMIZATION",
            "Optimize your work window",
            "You have " + to_string(highCount) +
            " high-priority task(s). Schedule them for tomorrow morning (2-4 PM peak focus window).",
            "Reschedule", "Peak focus", 80));
    }

    if (noDeadlineCount > 0) {
        addRecommendation(new Recommendation("DEADLINE ALERT",
            to_string(noDeadlineCount) + " task(s) without deadlines",
            "Tasks without deadlines often get postponed. Assign realistic deadlines to improve completion rate.",
            "Set deadlines", "Needs attention", 70));
    }

    if (taskCount >= 5) {
        addRecommendation(new Recommendation("PRODUCTIVITY",
            "You're on track with " + to_string(taskCount) + " tasks",
            "Your task list is well-organized. Keep maintaining this pace - consistency beats intensity.",
            "View insights", "Great progress", 90));
    }
}

string getTasksJson() {
    string json = "["; TaskNode* temp = head; bool first = true;
    while (temp) {
        if (!first) json += ",";
        json += "{\"title\":\"" + temp->task->title + "\",\"description\":\""
              + temp->task->description + "\",\"dueDate\":\"" + temp->task->dueDate
              + "\",\"priority\":\"" + temp->task->priority + "\"}";
        first = false; temp = temp->next;
    }
    json += "]"; return json;
}

string getProcessingJson() {
    bool isProcessing = false;
    TaskNode* temp = head;
    while (temp) {
        if (temp->task->status == "Processing") {
            isProcessing = true;
            int inc = rand() % 11 + 5;
            temp->task->progress += inc;
            if (temp->task->progress >= 100) {
                temp->task->progress = 100;
                if (rand() % 10 == 0) {
                    temp->task->status = "Failed";
                    temp->task->statusMsg = "Error: Invalid input format. Please retry.";
                } else {
                    temp->task->status = "Completed";
                    temp->task->statusMsg = "AI analysis finished successfully.";
                }
            } else temp->task->statusMsg = "Analyzing content...";
            break;
        }
        temp = temp->next;
    }

    if (!isProcessing) {
        temp = head;
        while (temp) {
            if (temp->task->status == "Queued") {
                temp->task->status = "Processing";
                temp->task->statusMsg = "Starting AI analysis...";
                break;
            }
            temp = temp->next;
        }
    }

    string json = "["; temp = head; bool first = true; int index = 1;
    while (temp) {
        if (!first) json += ",";
        string id = "PRC-20" + to_string(1000 + index);
        string tInfo = "Just now";
        if (temp->task->status == "Completed") tInfo = "Completed just now";
        if (temp->task->status == "Failed") tInfo = "Failed just now";
        json += "{\"id\":\"" + id + "\",\"title\":\"" + temp->task->title
              + "\",\"status\":\"" + temp->task->status + "\",\"progress\":"
              + to_string(temp->task->progress) + ",\"statusText\":\""
              + temp->task->statusMsg + "\",\"timeInfo\":\"" + tInfo + "\"}";
        first = false; index++; temp = temp->next;
    }
    json += "]"; return json;
}

string getRankedTasksJson(int uW, int iW, int eW) {
    TaskNode* temp = head;
    while (temp) { temp->task->score = calculateScore(temp->task, uW, iW, eW); temp = temp->next; }
    sortTasksByScore();
    string json = "["; temp = head; bool first = true; int rank = 1;
    while (temp) {
        if (!first) json += ",";
        json += "{\"rank\":" + to_string(rank) + ",\"title\":\"" + temp->task->title
              + "\",\"dueDate\":\"" + temp->task->dueDate + "\",\"priority\":\""
              + temp->task->priority + "\",\"score\":" + to_string(temp->task->score) + "}";
        first = false; rank++; temp = temp->next;
    }
    json += "]"; return json;
}

string getCalendarJson() {
    string json = "{";
    TaskNode* temp = head;
    bool firstDate = true;
    vector<string> dates;
    temp = head;
    while (temp) {
        string d = temp->task->dueDate;
        if (d != "No Deadline" && !d.empty() &&
            find(dates.begin(), dates.end(), d) == dates.end()) {
            dates.push_back(d);
        }
        temp = temp->next;
    }
    for (const string& date : dates) {
        if (!firstDate) json += ",";
        json += "\"" + date + "\":[";
        temp = head;
        bool firstTask = true;
        while (temp) {
            if (temp->task->dueDate == date) {
                if (!firstTask) json += ",";
                json += "{";
                json += "\"title\":\"" + temp->task->title + "\",";
                json += "\"priority\":\"" + temp->task->priority + "\",";
                json += "\"description\":\"" + temp->task->description + "\"";
                json += "}";
                firstTask = false;
            }
            temp = temp->next;
        }
        json += "]";
        firstDate = false;
    }
    json += "}";
    return json;
}

string getRecommendationsJson(int* totalConfidence, int* recCount) {
    generateRecommendations();
    string json = "["; RecNode* temp = recHead; bool first = true;
    int totalConf = 0, count = 0;
    while (temp) {
        if (!first) json += ",";
        json += "{";
        json += "\"type\":\"" + temp->rec->type + "\",";
        json += "\"title\":\"" + temp->rec->title + "\",";
        json += "\"description\":\"" + temp->rec->description + "\",";
        json += "\"action\":\"" + temp->rec->action + "\",";
        json += "\"meta\":\"" + temp->rec->meta + "\",";
        json += "\"confidence\":" + to_string(temp->rec->confidence);
        json += "}";
        totalConf += temp->rec->confidence; count++;
        first = false; temp = temp->next;
    }
    json += "]";
    *totalConfidence = (count > 0) ? (totalConf / count) : 0;
    *recCount = count;
    return json;
}

string extractJsonValue(const string& jsonStr, const string& key) {
    string searchKey = "\"" + key + "\":\"";
    size_t start = jsonStr.find(searchKey);
    if (start == string::npos) return "";
    start += searchKey.length();
    size_t end = jsonStr.find("\"", start);
    return jsonStr.substr(start, end - start);
}

bool extractJsonBool(const string& jsonStr, const string& key) {
    string searchKey = "\"" + key + "\":";
    size_t start = jsonStr.find(searchKey);
    if (start == string::npos) return false;
    start += searchKey.length();
    return jsonStr.substr(start, 4).find("true") != string::npos;
}

int main() {
    srand((unsigned)time(0));

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) { WSACleanup(); return 1; }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        closesocket(server_fd); WSACleanup(); return 1;
    }
    if (listen(server_fd, 5) == SOCKET_ERROR) {
        closesocket(server_fd); WSACleanup(); return 1;
    }

    cout << "===============================================\n";
    cout << "  TaskFlow AI - C++ Native Server\n";
    cout << "  Running on port 8080\n";
    cout << "  Waiting for connections from HTML...\n";
    cout << "===============================================\n" << endl;

    while (true) {
        SOCKET new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket == INVALID_SOCKET) continue;

        char buffer[65536] = {0};
        recv(new_socket, buffer, 65536, 0);
        string request(buffer);

        if (request.find("OPTIONS") == 0) {
            string response = "HTTP/1.1 200 OK\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Headers: Content-Type\r\n"
                              "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n\r\n";
            send(new_socket, response.c_str(), response.length(), 0);
            closesocket(new_socket); continue;
        }

        string corsHeaders = "Access-Control-Allow-Origin: *\r\n"
                             "Content-Type: application/json\r\n";

        if (request.find("POST /api/add-task") != string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != string::npos) {
                string body = request.substr(bodyPos + 4);
                Task* newTask = new Task(
                    extractJsonValue(body, "title"),
                    extractJsonValue(body, "description"),
                    extractJsonValue(body, "dueDate"),
                    extractJsonValue(body, "priority"));
                addTaskToList(newTask);
                logTaskVariadic(3, newTask->title.c_str(),
                                newTask->priority.c_str(),
                                newTask->dueDate.c_str());
                string jsonResponse = "{\"status\":\"success\", \"message\":\"Task saved to C++ backend!\"}";
                string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                                  "Content-Length: " + to_string(jsonResponse.length()) +
                                  "\r\n\r\n" + jsonResponse;
                send(new_socket, response.c_str(), response.length(), 0);
            }
        }
        else if (request.find("GET /api/get-tasks") != string::npos) {
            cout << "[Dashboard] tasks maange!" << endl;
            string jsonResponse = getTasksJson();
            string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                              "Content-Length: " + to_string(jsonResponse.length()) +
                              "\r\n\r\n" + jsonResponse;
            send(new_socket, response.c_str(), response.length(), 0);
        }
        else if (request.find("GET /api/get-processing") != string::npos) {
            cout << "[Processing] page ne data maanga!" << endl;
            string jsonResponse = getProcessingJson();
            string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                              "Content-Length: " + to_string(jsonResponse.length()) +
                              "\r\n\r\n" + jsonResponse;
            send(new_socket, response.c_str(), response.length(), 0);
        }
        else if (request.find("GET /api/prioritize") != string::npos) {
            int uW = 80, iW = 65, eW = 45;
            size_t uPos = request.find("u=");
            if (uPos != string::npos) uW = atoi(request.substr(uPos + 2, 3).c_str());
            size_t iPos = request.find("i=");
            if (iPos != string::npos) iW = atoi(request.substr(iPos + 2, 3).c_str());
            size_t ePos = request.find("e=");
            if (ePos != string::npos) eW = atoi(request.substr(ePos + 2, 3).c_str());
            cout << "[Priority Engine] u=" << uW << " i=" << iW << " e=" << eW << endl;
            string jsonResponse = getRankedTasksJson(uW, iW, eW);
            string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                              "Content-Length: " + to_string(jsonResponse.length()) +
                              "\r\n\r\n" + jsonResponse;
            send(new_socket, response.c_str(), response.length(), 0);
        }
        else if (request.find("GET /api/recommendations") != string::npos) {
            cout << "[Recommendations] page ne data maanga!" << endl;
            int totalConf = 0, recCount = 0;
            string recsJson = getRecommendationsJson(&totalConf, &recCount);
            string jsonResponse = "{";
            jsonResponse += "\"confidence\":" + to_string(totalConf) + ",";
            jsonResponse += "\"count\":" + to_string(recCount) + ",";
            jsonResponse += "\"recommendations\":" + recsJson;
            jsonResponse += "}";
            string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                              "Content-Length: " + to_string(jsonResponse.length()) +
                              "\r\n\r\n" + jsonResponse;
            send(new_socket, response.c_str(), response.length(), 0);
        }
        else if (request.find("GET /api/calendar") != string::npos) {
            cout << "[Calendar] page ne data maanga!" << endl;
            string jsonResponse = getCalendarJson();
            string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                              "Content-Length: " + to_string(jsonResponse.length()) +
                              "\r\n\r\n" + jsonResponse;
            send(new_socket, response.c_str(), response.length(), 0);
        }
        else if (request.find("GET /api/settings") != string::npos) {
            string json = "{";
            json += "\"fullName\":\""      + g_settings.fullName      + "\",";
            json += "\"email\":\""         + g_settings.email         + "\",";
            json += "\"theme\":\""         + g_settings.theme         + "\",";
            json += "\"language\":\""      + g_settings.language      + "\",";
            json += "\"timezone\":\""      + g_settings.timezone      + "\",";
            json += "\"priorityModel\":\"" + g_settings.priorityModel + "\",";
            json += "\"autoSchedule\":"   + string(g_settings.autoSchedule   ? "true" : "false") + ",";
            json += "\"smartReminders\":" + string(g_settings.smartReminders ? "true" : "false") + ",";
            json += "\"emailNotif\":"     + string(g_settings.emailNotif     ? "true" : "false") + ",";
            json += "\"pushNotif\":"      + string(g_settings.pushNotif      ? "true" : "false") + ",";
            json += "\"aiSuggestions\":"  + string(g_settings.aiSuggestions  ? "true" : "false");
            json += "}";
            cout << "[Settings] Data sent to client" << endl;
            string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                              "Content-Length: " + to_string(json.length()) +
                              "\r\n\r\n" + json;
            send(new_socket, response.c_str(), response.length(), 0);
        }
        else if (request.find("POST /api/settings") != string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != string::npos) {
                string body = request.substr(bodyPos + 4);
                string v;

                v = extractJsonValue(body, "fullName");
                if (!v.empty()) g_settings.fullName = v;
                v = extractJsonValue(body, "email");
                if (!v.empty()) g_settings.email = v;
                v = extractJsonValue(body, "theme");
                if (!v.empty()) g_settings.theme = v;
                v = extractJsonValue(body, "language");
                if (!v.empty()) g_settings.language = v;
                v = extractJsonValue(body, "timezone");
                if (!v.empty()) g_settings.timezone = v;
                v = extractJsonValue(body, "priorityModel");
                if (!v.empty()) g_settings.priorityModel = v;

                g_settings.autoSchedule   = extractJsonBool(body, "autoSchedule");
                g_settings.smartReminders = extractJsonBool(body, "smartReminders");
                g_settings.emailNotif     = extractJsonBool(body, "emailNotif");
                g_settings.pushNotif      = extractJsonBool(body, "pushNotif");
                g_settings.aiSuggestions  = extractJsonBool(body, "aiSuggestions");

                cout << "[Settings] Updated -> " << g_settings.fullName
                     << " | theme=" << g_settings.theme
                     << " | lang="  << g_settings.language
                     << " | tz="    << g_settings.timezone << endl;

                string json = "{\"status\":\"success\",\"message\":\"Settings saved!\"}";
                string response = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                                  "Content-Length: " + to_string(json.length()) +
                                  "\r\n\r\n" + json;
                send(new_socket, response.c_str(), response.length(), 0);
            }
        }

        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}