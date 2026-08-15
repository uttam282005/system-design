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

class TaskNotFound : public std::domain_error {};

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
