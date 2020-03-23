// lab1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <list>
#include <Windows.h>
using namespace std;

struct MemoryBlock {    
	bool is_free;
	short size;
	void* ptr;
	MemoryBlock* next_block;
	MemoryBlock* prev_block;
};

struct MemoryArea {
	void* ptr;
	MemoryBlock* first_mem_block;
	bool has_free_block;
};

class Allocator {

private:
	short memory_area_size;
	list<MemoryArea*> mem_areas;
	~Allocator() {
		for (auto it = mem_areas.begin(); it != mem_areas.end(); ++it) {
			HeapDestroy((*it)->ptr);
		}
	}


public:
	Allocator() {
		memory_area_size = 64;   //default value for my algorithm
		create_memory_area();
	}

	Allocator(short memory_area_size) {    //constructor for setting size value by user
		this->memory_area_size = memory_area_size;
		create_memory_area();
	}

	void *mem_alloc(size_t size) {
		size = align_size(size);
		bool founded;
		size_t block_size = 0;
		MemoryBlock* block = findBlock(block_size, size, founded);
		if (founded) {
			*block = *(allocate(block_size, size, block));
			return block->ptr;
		}
		create_memory_area();
		MemoryBlock* new_block = mem_areas.back()->first_mem_block;
		*new_block = *(allocate(memory_area_size, size, new_block));
		return new_block->ptr;
	}

	void *mem_realloc(void *addr, size_t size) {
		size = align_size(size);
		if (addr == nullptr)
			return mem_alloc(size);
		if (size > memory_area_size)
			return addr;
		else {
			bool founded;
			size_t block_size;
			MemoryArea* mem_area;
			void* ptr = mem_alloc(size);
			get_block_info(mem_area, ptr);
			memcpy(ptr,addr, size);
			mem_free(addr);
			go_to_stack_start(*(mem_area->first_mem_block));
		}
	}
	void mem_free(void *addr) {
		MemoryArea* mem_area;
		MemoryBlock* block = get_block_info(mem_area, addr);
		if (mem_area != nullptr) {
			void* heap_addr = mem_area->ptr;
			break_connections(*block, mem_area);
			HeapFree(heap_addr, HEAP_NO_SERIALIZE, addr);
		}
	}

	void mem_dump() {
		int i = 1;
		for (auto it = mem_areas.begin(); it != mem_areas.end(); ++it) {
			MemoryBlock* block = (*it)->first_mem_block;
			cout << "\nArea number " << i++ << "\n_______________________________________\n";
			while (block) {
				cout << "block with pointer: " << block->ptr << " size: " << block->size << " status: ";
				if (block->is_free)
					cout << "free\n";
				else
					cout << "occupied\n";
				block = block->next_block;
			}
		}
	}
private:
	void create_memory_area() {
		MemoryArea* mem_area = new MemoryArea();
		mem_area->ptr = HeapCreate(HEAP_NO_SERIALIZE, memory_area_size, memory_area_size);
		MemoryBlock* block = new MemoryBlock();
		block->is_free = true;
		block->ptr = mem_area->ptr;
		block->size = memory_area_size;
		block->next_block = nullptr;
		block->prev_block = nullptr;
		mem_area->first_mem_block = block;
		mem_area->has_free_block = true;
		mem_areas.push_back(mem_area);
	}

	MemoryBlock* findBlock(size_t &block_size, size_t &size, bool &founded)
	{
		if (size > memory_area_size)
			return nullptr;
		for (auto memory_area = mem_areas.begin(); memory_area != mem_areas.end(); ++memory_area) {
			bool retflag;
			MemoryBlock * retval = find_block_in_area(*memory_area, founded, block_size, size);
			if (founded) return retval;
		}
		founded = false;
		return nullptr;
	}

	MemoryBlock * find_block_in_area(MemoryArea * memory_area, bool& founded, size_t & block_size, size_t & size)
	{
		if (memory_area->has_free_block) {
			MemoryBlock* block = memory_area->first_mem_block;
			bool has_free_block = false;
			while (block != nullptr) {
				if (block->is_free) {
					has_free_block = true;
					block_size = block->size;
					if (block_size >= size) {
						founded = true;
						return block;
					}
				}
				block = block->next_block;
			}
			memory_area->has_free_block = has_free_block;
		}
		founded = false;
		return nullptr;
	}

	MemoryBlock* allocate(short block_size, const size_t &size, MemoryBlock* block)
	{
		try {
			MemoryBlock* new_block = new MemoryBlock();
			new_block->ptr = HeapAlloc(block->ptr, HEAP_NO_SERIALIZE, size);
			new_block->is_free = false;
			new_block->size = size;
			add_connections(block, new_block, block_size, size);
			return new_block;
		}
		catch (exception e) {
			return new MemoryBlock();
		}
	}

	void add_connections(MemoryBlock * block, MemoryBlock * new_block, short block_size, const size_t & size)
	{
		MemoryBlock* prev_block = block->prev_block;
		if (prev_block != nullptr) {
			prev_block->next_block = new_block;
		}
		new_block->prev_block = prev_block;
		if (block_size != size) {
			MemoryBlock* rest = new MemoryBlock();
			rest->ptr = block->ptr;
			rest->is_free = true;
			rest->size = block_size - size;
			rest->next_block = block->next_block;
			rest->prev_block = new_block;
			new_block->next_block = rest;
		}
	}

	size_t align_size(size_t size) {
		return (size + 3) & ~(3);
	}

	MemoryBlock* get_block_info(MemoryArea * & area, void * addr)
	{
		for (auto mem_area = mem_areas.begin(); mem_area != mem_areas.end(); ++mem_area) {
			area = *mem_area;
			MemoryBlock& block = *(area->first_mem_block);
			while ((&block) != nullptr) {
				if ((&block)->ptr == addr) {
					return &block;
				}
				MemoryBlock* next_block = (&block)->next_block;
				if (next_block != nullptr)
					block = *next_block;
				else
					break;
			}
		}
		return nullptr;
	}

	void break_connections(MemoryBlock& block, MemoryArea* & area)
	{
		MemoryBlock& next_block = *((&block)->next_block);
		if ((&block)->next_block != nullptr) {
			(&block)->next_block->prev_block = (&block)->prev_block;
			if ((&block)->next_block->prev_block != nullptr)
				(&block)->next_block->prev_block->next_block = (&block)->next_block;
			else
				area->first_mem_block = (&block)->next_block;
		}
		else {
			if ((&block)->prev_block != nullptr)
				(&block)->prev_block->next_block = nullptr;
			else
				area->first_mem_block = nullptr;
		}
		go_to_stack_start(block);
	}

	void go_to_stack_start(MemoryBlock & block)
	{
		while ((&block)->prev_block != nullptr)
			block = *((&block)->prev_block);
	}
};

int main()
{
	Allocator* alloc = new Allocator();
	cout << "______________________________________________\n";
	cout << "Allocated memory blocks with size 25 and 39\n";
	cout << "______________________________________________\n";
	void* addr1 = alloc->mem_alloc(25);
	void* addr2 = alloc->mem_alloc(39);
	alloc->mem_dump();
	cout << "______________________________________________\n";
	cout << "\nReallocated memory block " << addr2 << " to memory block with size 18\n";
	cout << "______________________________________________\n";
	alloc->mem_realloc(addr2, 18);
	alloc->mem_dump();
	cout << "______________________________________________\n";
	cout << "\nFreed memory block " << addr1 << "\n";
	cout << "______________________________________________\n";
	alloc->mem_free(addr1);
	alloc->mem_dump();
	system("pause");
}
