#include <string>
#include <vector>
#include <iostream>
#include <initializer_list>

using namespace std;

class SqlQuery {
private:
    string table;
    vector<string> columns;
    vector<string> conditions;
    string orderByCol;
    string orderDir;
    int limitVal;
    int offsetVal;

    SqlQuery(const string& table, const vector<string>& columns,
             const vector<string>& conditions, const string& orderByCol,
             const string& orderDir, int limitVal, int offsetVal)
        : table(table), columns(columns), conditions(conditions),
          orderByCol(orderByCol), orderDir(orderDir),
          limitVal(limitVal), offsetVal(offsetVal) {}

public:
    string toSql() const {
        string sql = "SELECT ";
        if (columns.empty()) {
            sql += "*";
        } else {
            for (size_t i = 0; i < columns.size(); i++) {
                if (i > 0) sql += ", ";
                sql += columns[i];
            }
        }
        sql += " FROM " + table;
        if (!conditions.empty()) {
            sql += " WHERE ";
            for (size_t i = 0; i < conditions.size(); i++) {
                if (i > 0) sql += " AND ";
                sql += conditions[i];
            }
        }
        if (!orderByCol.empty()) {
            sql += " ORDER BY " + orderByCol + " " + orderDir;
        }
        if (limitVal > 0) sql += " LIMIT " + to_string(limitVal);
        if (offsetVal > 0) sql += " OFFSET " + to_string(offsetVal);
        return sql;
    }

    class Builder {
    private:
        string table;
        vector<string> columns;
        vector<string> conditions;
        string orderByCol;
        string orderDir = "ASC";
        int limitVal = 0;
        int offsetVal = 0;

    public:
        explicit Builder(const string& table) : table(table) {}

        Builder& select(initializer_list<string> cols) {
            columns.insert(columns.end(), cols.begin(), cols.end());
            return *this;
        }

        Builder& where(const string& condition) {
            conditions.push_back(condition);
            return *this;
        }

        Builder& orderBy(const string& col, const string& dir) {
            orderByCol = col;
            orderDir = dir;
            return *this;
        }

        Builder& limit(int l) {
            limitVal = l;
            return *this;
        }
        Builder& offset(int o) {
            offsetVal = o;
            return *this;
        }

        SqlQuery build() const {
            return SqlQuery(table, columns, conditions,
                            orderByCol, orderDir, limitVal, offsetVal);
        }
    };
};

int main() {
    auto query1 = SqlQuery::Builder("users")
        .select({"name", "email"})
        .where("age > 18")
        .where("active = true")
        .orderBy("name", "ASC")
        .limit(10)
        .build();

    auto query2 = SqlQuery::Builder("orders")
        .select({"id", "total", "created_at"})
        .where("status = 'completed'")
        .where("total > 100")
        .orderBy("created_at", "DESC")
        .limit(20)
        .offset(40)
        .build();

    cout << query1.toSql() << endl;
    cout << query2.toSql() << endl;

    return 0;
}
