#pragma once

#include "KeyAssignment.h"
#include <utility>

/** Editor-session history. A range copy is one edit, even when it touches 128 keys. */
class AssignmentHistory
{
public:
    struct Edit
    {
        int selectedKey;
        std::vector<std::pair<int, KeyAssignment>> assignments;
    };

    void record(Edit edit)
    {
        push(undoEdits, std::move(edit));
        redoEdits.clear();
    }
    void clear() { undoEdits.clear(); redoEdits.clear(); }
    bool canUndo() const { return !undoEdits.empty(); }
    bool canRedo() const { return !redoEdits.empty(); }

    template <typename Get, typename Set>
    int undo(Get get, Set set) { return restore(undoEdits, redoEdits, get, set); }
    template <typename Get, typename Set>
    int redo(Get get, Set set) { return restore(redoEdits, undoEdits, get, set); }

private:
    std::vector<Edit> undoEdits, redoEdits;
    static void push(std::vector<Edit>& edits, Edit edit)
    {
        if (edits.size() >= 100) edits.erase(edits.begin());
        edits.push_back(std::move(edit));
    }
    template <typename Get, typename Set>
    static int restore(std::vector<Edit>& from, std::vector<Edit>& to, Get get, Set set)
    {
        if (from.empty()) return -1;
        auto edit = std::move(from.back());
        from.pop_back();
        Edit inverse { edit.selectedKey, {} };
        for (const auto& [key, assignment] : edit.assignments)
        {
            inverse.assignments.emplace_back(key, get(key));
            set(key, assignment);
        }
        push(to, std::move(inverse));
        return edit.selectedKey;
    }
};
