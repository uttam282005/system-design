#include <stdexcept>
#include <string>
#include <memory>
#include <bits/stdc++.h>

enum class TaskStatus {
    TODO,
    IN_PROGRESS,
    DONE,
    CANCELLED
};

enum class Priority {
    LOW,
    MEDIUM,
    HIGH
};

class TaskNotFound : public std::domain_error {
public:
    TaskNotFound() : std::domain_error("Task not found") {}
};

class UserNotFound : public std::domain_error {
public:
    UserNotFound() : std::domain_error("User not found") {}
};

class User {
private:
    int id;
    std::string name;

public:
    User(int id, std::string name)
        : id(id), name(std::move(name)) {}

    int getId() const {
        return id;
    }

    const std::string& getName() const {
        return name;
    }
};

class Comment {
private:
    int id;
    int authorId;
    std::string content;

public:
    Comment(
        int id,
        int authorId,
        std::string content
    )
        : id(id),
          authorId(authorId),
          content(std::move(content)) {}

    int getId() const {
        return id;
    }

    int getAuthorId() const {
        return authorId;
    }

    const std::string& getContent() const {
        return content;
    }
};

class Task {
private:
    int id;
    std::string title;
    std::string description;

    TaskStatus status;
    Priority priority;

    std::optional<int> assigneeId;

    std::vector<std::shared_ptr<Task>> subtasks;
    std::vector<Comment> comments;

public:
    Task(
        int id,
        std::string title,
        std::string description,
        Priority priority
    )
        : id(id),
          title(std::move(title)),
          description(std::move(description)),
          status(TaskStatus::TODO),
          priority(priority) {}

    int getId() const {
        return id;
    }

    TaskStatus getStatus() const {
        return status;
    }

    Priority getPriority() const {
        return priority;
    }

    void changeStatus(TaskStatus newStatus) {
        if (!isValidTransition(status, newStatus)) {
            throw std::invalid_argument(
                "Invalid status transition"
            );
        }

        status = newStatus;
    }

    void setPriority(Priority newPriority) {
        priority = newPriority;
    }

    void assignTo(int userId) {
        assigneeId = userId;
    }

    void unassign() {
        assigneeId.reset();
    }

    void addSubtask(std::shared_ptr<Task> task) {
        subtasks.push_back(std::move(task));
    }

    void addComment(Comment comment) {
        comments.push_back(std::move(comment));
    }

private:
    static bool isValidTransition(
        TaskStatus from,
        TaskStatus to
    ) {
        if (from == TaskStatus::DONE ||
            from == TaskStatus::CANCELLED) {
            return false;
        }

        return true;
    }
};

class ITaskRepository {
public:
    virtual ~ITaskRepository() = default;

    virtual std::shared_ptr<Task>
    findById(int id) = 0;

    virtual void save(
        std::shared_ptr<Task> task
    ) = 0;

    virtual void remove(int id) = 0;
};

class InMemoryTaskRepository
    : public ITaskRepository {

private:
    std::unordered_map<
        int,
        std::shared_ptr<Task>
    > tasks;

public:
    std::shared_ptr<Task>
    findById(int id) override;

    void save(
        std::shared_ptr<Task> task
    ) override;

    void remove(int id) override;
};

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual std::optional<User>
    findById(int id) = 0;
};

class TaskService {
private:
    ITaskRepository& taskRepository;
    IUserRepository& userRepository;

public:
    TaskService(
        ITaskRepository& taskRepository,
        IUserRepository& userRepository
    )
        : taskRepository(taskRepository),
          userRepository(userRepository) {}

    int createTask(
        const std::string& title,
        const std::string& description,
        Priority priority
    );

    void assignTask(
        int taskId,
        int userId
    );

    void changeStatus(
        int taskId,
        TaskStatus newStatus
    );

    void deleteTask(
        int taskId
    );
};

void TaskService::assignTask(int taskId, int userId) {
    auto task = taskRepository.findById(taskId);

    if (!task)
        throw TaskNotFound{};

    auto user = userRepository.findById(userId);

    if (!user)
        throw UserNotFound{};

    task->assignTo(userId);

    taskRepository.save(task);
}

std::shared_ptr<Task>
InMemoryTaskRepository::findById(int id) {
    auto it = tasks.find(id);
    if (it == tasks.end())
        return nullptr;
    return it->second;
}

void InMemoryTaskRepository::save(
    std::shared_ptr<Task> task
) {
    tasks[task->getId()] = std::move(task);
}

void InMemoryTaskRepository::remove(int id) {
    tasks.erase(id);
}

int TaskService::createTask(
    const std::string& title,
    const std::string& description,
    Priority priority
) {
    static int nextId = 1;

    auto task = std::make_shared<Task>(
        nextId++,
        title,
        description,
        priority
    );

    taskRepository.save(task);

    return task->getId();
}

void TaskService::changeStatus(
    int taskId,
    TaskStatus newStatus
) {
    auto task = taskRepository.findById(taskId);

    if (!task)
        throw TaskNotFound{};

    task->changeStatus(newStatus);

    taskRepository.save(task);
}

void TaskService::deleteTask(int taskId) {
    auto task = taskRepository.findById(taskId);

    if (!task)
        throw TaskNotFound{};

    taskRepository.remove(taskId);
}

class InMemoryUserRepository
    : public IUserRepository {

private:
    std::unordered_map<int, User> users;

public:
    void add(User user) {
        users.emplace(
            user.getId(),
            std::move(user)
        );
    }

    std::optional<User>
    findById(int id) override {
        auto it = users.find(id);
        if (it == users.end())
            return std::nullopt;
        return it->second;
    }
};

const char* statusName(TaskStatus status) {
    switch (status) {
        case TaskStatus::TODO: return "TODO";
        case TaskStatus::IN_PROGRESS: return "IN_PROGRESS";
        case TaskStatus::DONE: return "DONE";
        case TaskStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

const char* priorityName(Priority priority) {
    switch (priority) {
        case Priority::LOW: return "LOW";
        case Priority::MEDIUM: return "MEDIUM";
        case Priority::HIGH: return "HIGH";
    }
    return "UNKNOWN";
}

int main() {
    InMemoryTaskRepository taskRepo;
    InMemoryUserRepository userRepo;

    userRepo.add(User(1, "Alice"));
    userRepo.add(User(2, "Bob"));

    TaskService service(taskRepo, userRepo);

    int taskId = service.createTask(
        "Implement login",
        "Add authentication flow",
        Priority::HIGH
    );
    std::cout << "Created task " << taskId << "\n";

    service.assignTask(taskId, 1);
    std::cout << "Assigned task " << taskId
              << " to Alice\n";

    service.changeStatus(
        taskId,
        TaskStatus::IN_PROGRESS
    );
    std::cout << "Status -> "
              << statusName(
                     taskRepo.findById(taskId)
                         ->getStatus()
                 )
              << "\n";

    service.changeStatus(taskId, TaskStatus::DONE);
    std::cout << "Status -> DONE\n";

    try {
        service.assignTask(999, 1);
    } catch (const TaskNotFound&) {
        std::cout << "assignTask(999) threw "
                     "TaskNotFound\n";
    }

    try {
        service.assignTask(taskId, 999);
    } catch (const UserNotFound&) {
        std::cout << "assignTask(..., 999) threw "
                     "UserNotFound\n";
    }

    try {
        service.changeStatus(
            taskId,
            TaskStatus::CANCELLED
        );
    } catch (const std::invalid_argument& e) {
        std::cout << "Invalid transition: "
                  << e.what() << "\n";
    }

    service.deleteTask(taskId);
    std::cout << "Deleted task " << taskId << "\n";

    try {
        service.changeStatus(
            taskId,
            TaskStatus::TODO
        );
    } catch (const TaskNotFound&) {
        std::cout << "changeStatus on deleted task "
                     "threw TaskNotFound\n";
    }

    return 0;
}
