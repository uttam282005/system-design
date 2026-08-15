// funcitonal requirements

// 1. The task management system should allow users to create, update, and
// delete tasks.
// 2. Each task should have a title, description, due date, priority, and status
// (e.g., pending, in progress, completed).
// 3. Users should be able to assign tasks to other users and set reminders for
// tasks.
// 4. The system should support searching and filtering tasks based on various
// criteria (e.g., priority, due date, assigned user).
// 5. Users should be able to mark tasks as completed and view their task
// history.
// 6. The system should handle concurrent access to tasks and ensure data
// consistency.
// 7. The system should be extensible to accommodate future enhancements and new
// features.

// non-functional requirements
//
// - data store shoudl be configurable

// core entities
//
// Task
// User
// Datastore
//
//
// realtionship
//
// User -> crates-> Task ->stored-> Datastore
// Datastore contains tasks (weak composition/association)
// User owns tasks (strong composition)
//
//
// classes and inerfaces

#include <ctime>
#include <string>

enum Status { PENDING, IN_PROGRESS, COMPLETED };
class User {};

class Task {
    inline static int taskID = 0;
public:
    Status status;
    std::tm due_date;
    std::string name;
    std::string description;

    Task(std::string name, std::string description, std::tm due_date) : name(name), description(description), due_date(due_date){ taskID++; }
};

class User::User {
public:
    const unsigned int userID;
    std::string name;
};

class Datastore {
    virtual bool _add() = 0;
    virtual bool _delete() = 0;
    virtual bool _getByID() = 0;

    virtual ~Datastore() = 0;
};


