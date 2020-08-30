#pragma once

#include "util.h"

template <typename T>
class node
{

public:

	T data;
	node* next;
	node* prev;

	node(T data) : data(data), next(nullptr), prev(nullptr)
	{

	}

	~node()
	{

	}

	void* operator new(size_t size)
	{
		return nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		nvhr_free(address);
	}

};

template <typename T>
class list
{

private:

	node<T>* head;
	node<T>* tail;
	size_t size;

public:

	list() : head(nullptr), tail(nullptr), size(0)
	{

	}

	~list()
	{

	}

	node<T>* get_head()
	{
		return this->head;
	}

	node<T>* get_tail()
	{
		return this->tail;
	}

	node<T>* add_head(T cell)
	{
		this->size++;
		node<T>* new_node = new node<T>(cell);
		if (this->head)
		{
			this->head->prev = new_node;
			new_node->next = this->head;
			new_node->prev = nullptr;
			this->head = new_node;
		}
		else
		{
			this->head = new_node;
			this->tail = new_node;
		}
		return new_node;
	}

	node<T>* add_tail(T cell)
	{
		this->size++;
		node<T>* new_node = new node<T>(cell);
		if (this->head)
		{
			this->tail->next = new_node;
			new_node->prev = this->tail;
			new_node->next = nullptr;
			this->tail = new_node;
		}
		else
		{
			this->head = new_node;
			this->tail = new_node;
		}
		return new_node;
	}

	node<T>* insert_before(node<T>* position, T cell)
	{
		if (this->head == position)
		{
			return this->add_head(cell);
		}
		else if (position == nullptr)
		{
			return this->add_tail(cell);
		}
		else
		{
			this->size++;
			node<T>* new_node = new node<T>(cell);
			new_node->prev = position->prev;
			position->prev = new_node;
			new_node->next = position;
			if (new_node->prev)
			{
				new_node->prev->next = new_node;
			}
			return new_node;
		}
	}

	void remove_head()
	{	
		if (this->head)
		{
			this->size--;
			node<T>* old_head = this->head;
			if (this->head == this->tail)
			{
				this->head = nullptr;
				this->tail = nullptr;
			}
			else
			{
				this->head = this->head->next;
				this->head->prev = nullptr;
			}
			delete old_head;
		}
	}

	void remove_tail()
	{
		if (this->tail)
		{
			this->size--;
			node<T>* old_tail = this->tail;
			if (this->head == this->tail)
			{
				this->head = nullptr;
				this->tail = nullptr;
			}
			else
			{
				this->tail = this->tail->prev;
				this->tail->next = nullptr;
			}
			delete old_tail;
		}
	}

	void remove_node(node<T>* node)
	{	
		if (node == this->head)
		{
			this->remove_head();
		}
		else if (node == this->tail)
		{
			this->remove_tail();
		}
		else
		{
			this->size--;
			node->prev->next = node->next;
			node->next->prev = node->prev;
			delete node;
		}
	}

	size_t get_size()
	{
		return this->size;
	}

	bool is_empty()
	{
		return !this->size;
	}

	void* operator new(size_t size)
	{
		return nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		nvhr_free(address);
	}

};
