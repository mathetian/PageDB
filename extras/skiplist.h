#ifndef _SKIP_LIST_H
#define _SKIP_LIST_H

#include <assert.h>
#include <stdlib.h>

#include <string>
using namespace std;
#include <time.h>

#define Key string

class SkipList
{
private:
    struct Node;
public:
    SkipList();
    ~SkipList() {}

public:
    void Insert(const Key & key);
    bool Remove(const Key & key);
    bool Contains(const Key & key) const;
    Key  Search(const Key & key);

public:
    class Iterator
    {
    public:
        Iterator(const SkipList* list);

    public:
        bool Valid() const;
        const Key& key() const;
        void Next();
        void Prev();
        void Seek(const Key& target);
        void SeekToFirst();
        void SeekToLast();

    private:
        const SkipList* list_;
        Node * node_;
    };

private:
    int GetMaxHeight() const
    {
        return max_height_;
    }
    bool Equal(const Key& a, const Key& b) const
    {
        return a == b;
    }

    Node* NewNode(const Key& key, int height);
    int  RandomHeight();

    bool KeyIsAfterNode(const Key& key, Node* n) const;
    Node* FindGreaterOrEqual(const Key& key, Node** prev) const;
    Node* FindLessThan(const Key& key) const;
    Node* FindLast() const;
    SkipList(const SkipList&);
    void operator=(const SkipList&);

private:
    enum { kMaxHeight = 12 };
    int max_height_;
    struct Node;
    Node*  head_;
    int    count_;
};

struct SkipList::Node
{
    Node(const Key& k) : key(k) { }

    Key key;

    Node* Next(int n)
    {
        return next_[n];
    }

    void SetNext(int n, Node* x)
    {
        next_[n] = x;
    }

    void SetKey(const Key & k)
    {
        key = k;
    }

private:
    Node * next_[1];
};

SkipList::Node * SkipList::NewNode(const Key& key, int height)
{
    size_t size = sizeof(Node) + sizeof(Node*) * (height - 1);
    char* alloc_ptr = new char[size];
    return new (alloc_ptr) Node(key);
}

SkipList::Iterator::Iterator(const SkipList* list)
{
    list_ = list;
    node_ = NULL;
}

bool SkipList::Iterator::Valid() const
{
    return node_ != NULL;
}

const Key& SkipList::Iterator::key() const
{
    return node_->key;
}

void SkipList::Iterator::Next()
{
    node_ = node_->Next(0);
}

void SkipList::Iterator::Prev()
{
    node_ = list_->FindLessThan(node_->key);
    if (node_ == list_->head_)
        node_ = NULL;
}

void SkipList::Iterator::Seek(const Key& target)
{
    node_ = list_->FindGreaterOrEqual(target, NULL);
    if (node_ == list_->head_)
        node_ = NULL;
}

void SkipList::Iterator::SeekToFirst()
{
    node_ = list_->head_->Next(0);
}

void SkipList::Iterator::SeekToLast()
{
    Node* curr = list_->head_;
    size_t level = list_->max_height_ - 1;

    while (true)
    {
        Node* next = curr->Next(level);

        if (next == NULL)
        {
            if (level == 0)
                break;
            else
                level--;
        }
        else
        {
            curr = next;
        }
    }

    node_ = curr;
    if (node_ == list_->head_)
        node_ = NULL;
}

int SkipList::RandomHeight()
{
    static const size_t kBranching = 4;
    int height = 1;

    while (height < kMaxHeight && (rand() % kBranching) == 0)
        height++;

    return height;
}

SkipList::Node * SkipList::FindGreaterOrEqual(const Key& key, Node** prev) const
{
    Node* curr = head_;
    size_t level = max_height_ - 1;

    while (true)
    {
        Node* next = curr->Next(level);

        if (next != NULL && next -> key < key)
        {
            curr = next;
        }
        else
        {
            if (prev != NULL)
                prev[level] = curr;

            if (level == 0)
                return next;
            else
                level--;
        }
    }
}

SkipList::Node * SkipList::FindLessThan(const Key& key) const
{
    Node* curr = head_;
    size_t level = max_height_ - 1;

    while (true)
    {
        Node* next = curr->Next(level);

        if (next == NULL || next->key >= key )
        {
            if (level == 0)
                return curr;
            else
                level--;
        }
        else
        {
            curr = next;
        }
    }
}

SkipList::SkipList()
{
    srand(time(0));
    head_ = NewNode(Key(), kMaxHeight);
    max_height_ = 1;
    count_ = 0;
    for (int i = 0; i < kMaxHeight; i++)
        head_->SetNext(i, NULL);
}

void SkipList::Insert(const Key & key)
{
    Node* prev[kMaxHeight];

    Node* next = FindGreaterOrEqual(key, prev);

    size_t height = RandomHeight();

    if (height > max_height_)
    {
        for (size_t i = max_height_; i < height; i++)
            prev[i] = head_;

        max_height_ = height;
    }

    if (next && next->key == key)
    {
        next->SetKey(key);
    }
    else
    {
        Node* curr = NewNode(key, height);

        for (size_t i = 0; i < height; i++)
        {
            curr->SetNext(i, prev[i]->Next(i));
            prev[i]->SetNext(i, curr);
        }

        count_++;
    }
}

bool SkipList::Contains(const Key& key) const
{
    Node* x = FindGreaterOrEqual(key, NULL);

    if (x != NULL && x-> key == key)
        return true;
    else
        return false;
}

bool SkipList::Remove(const Key& key)
{
    Node* prev[kMaxHeight];
    Node* curr = FindGreaterOrEqual(key, prev);

    assert(curr != NULL);
    assert(Equal(curr->key, key));

    for (size_t i = 0; i < max_height_; i++)
    {
        if (prev[i]->Next(i) == curr)
            prev[i]->SetNext(i, curr->Next(i));
    }

    count_--;
}

#endif