// lab2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <list>
#include <string>
#include <Windows.h>
#include <sstream>
using namespace std;

struct MemoryBlock {
	bool is_free;
	short size;
	void* ptr;
	MemoryBlock* next_block;
	MemoryBlock* prev_block;
};

struct MemoryPage {
	void* ptr;
	MemoryBlock* first_free_block;
	string page_class;
	MemoryPage* next_page;
};

struct MemoryArea {
	void* ptr;
	list<MemoryPage*> pages;
};


class Allocator {

private:
	int memory_area_size;
	short page_size;
	list<MemoryArea*> mem_areas;
	list<MemoryPage*> free_pages;
	~Allocator() {
		for (auto it = mem_areas.begin(); it != mem_areas.end(); ++it) {
			HeapDestroy((*it)->ptr);
		}
	}


public:
	Allocator() {
		memory_area_size = 20480;   //default value for memory area (20 KB)
		page_size = 4096;            //default value for memory page (4 KB)
		create_memory_area();
	}

	Allocator(short memory_area_size) {    //constructor for setting size value by user
		this->memory_area_size = memory_area_size;
		create_memory_area();
	}

	void *mem_alloc(size_t size) {
		size = align_size(size);
		if (size >= page_size / 2) {
			MemoryPage* firstPage = nullptr;
			MemoryPage* prev_page = nullptr;
			for (auto page = free_pages.begin(); page != free_pages.end(); ++page) {
				if ((*page)->page_class == "free") {
					(*page)->page_class = "occupied";
					(*page)->next_page = nullptr;
					if (prev_page != nullptr)
						prev_page->next_page = *page;
					else
						firstPage = *page;
					if (size > page_size) {
						size -= page_size;
						prev_page = *page;
						continue;
					}
					else break;
				}
			}
			void* ptr = firstPage->ptr;
			while (firstPage != nullptr){
				free_pages.remove(firstPage);
				firstPage = firstPage->next_page;
			}
			return ptr;
		}
		else {
			bool founded;
			MemoryBlock* block = findBlock(size, founded);
			if (!founded) {
				block = transorm_page_and_return__free_block(size);
			}
			return block->ptr;
		}
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
			//get_block_info(mem_area, ptr);
			memcpy(ptr, addr, size);
			mem_free(addr);
			//go_to_stack_start(*(mem_area->first_mem_block));
		}
	}
	void mem_free(void *addr) {
		MemoryPage* mem_page;
		MemoryBlock* block;
		get_block_info(mem_page, block, addr);
		if (mem_page != nullptr) {
				void* heap_addr = mem_page->ptr;
				break_connections(block, mem_page);
		}
	}

	void mem_dump() {
		int i = 1;
		for (auto it = mem_areas.begin(); it != mem_areas.end(); ++it) {
			list<MemoryPage*> pages = (*it)->pages;
			cout << "\nArea number " << i++ << "\n_______________________________________\n";
			for (auto page = pages.begin(); page != pages.end(); ++page) {
				cout << "page ptr: " << (*page)->ptr << " | class: " << (*page)->page_class;
				cout << " | next_page: " << (*page)->next_page << "\n";
				MemoryBlock* block = nullptr;
				memcpy(&block, &((*page)->first_free_block), sizeof((*page)->first_free_block));
				if (block != nullptr) {
					cout << "Blocks:\n";
					block = go_to_stack_start(block);
					while (block != nullptr) {
						cout << "           pointer: " << block->ptr << " size: " << block->size << " status: ";
						if (block->is_free)
							cout << "free\n";
						else
							cout << "occupied\n";
						if (block->next_block == nullptr)
							break;
						else
							block = block->next_block;
					}
					block = go_to_stack_start(block);
				}
			}
		}
	}
private:
	void create_memory_area() {
		MemoryArea* mem_area = new MemoryArea();
		void* pointer = HeapCreate(HEAP_NO_SERIALIZE, memory_area_size, memory_area_size);
		mem_area->ptr = pointer;
		for (int i = 0; i < memory_area_size / page_size; i++) {
			MemoryPage * page = new MemoryPage();
			page->ptr = HeapAlloc(mem_area->ptr, HEAP_NO_SERIALIZE, page_size);
			page->page_class = "free";
			mem_area->pages.push_back(page);
			free_pages.push_back(page);
		}
		mem_areas.push_back(mem_area);
	}


	MemoryBlock* transorm_page_and_return__free_block(size_t size) {
		for (auto it = free_pages.begin(); it != free_pages.end(); ++it) {
			if ((*it)->page_class == "free") {
				void* pointer = HeapCreate(HEAP_NO_SERIALIZE, page_size, page_size);
				MemoryBlock* prev_block = nullptr;
				for (int i = 0; i < page_size / size; i++) {
					MemoryBlock* block = new MemoryBlock();
					block->is_free = true;
					block->prev_block = prev_block;
					block->size = size;
					block->ptr = HeapAlloc(pointer, HEAP_NO_SERIALIZE, size);
					if (prev_block != nullptr)
						prev_block->next_block = block;
					else
						block->is_free = false;
					prev_block = block;
				}
				prev_block = go_to_stack_start(prev_block);
				(*it)->page_class = to_string(size) + "B";
				(*it)->first_free_block = prev_block->next_block;
				return prev_block;
			}
		}
	}

	MemoryBlock* findBlock(size_t &size, bool &founded)
	{
		if (size > memory_area_size)
			return nullptr;
		for (auto it = free_pages.begin(); it != free_pages.end(); ++it) {
			int block_size;
			string page_class = (*it)->page_class;
			std::istringstream sizestr(page_class.substr(0, page_class.length() - 1));
			sizestr >> block_size;
			if (block_size - size >= 0 && block_size - size < 4) {
				MemoryBlock* block = (*it)->first_free_block;
				block->is_free = false;
				if (block->next_block != nullptr)
					(*it)->first_free_block = block->next_block;
				else
					(*it)->page_class = "occupied";
				founded = true;
				return block;
			}
		}
		founded = false;
		return nullptr;
	}


	size_t align_size(size_t size) {
		return (size + 3) & ~(3);
	}

	void get_block_info(MemoryPage * & page, MemoryBlock* & block, void * addr)
	{
		for (auto mem_area = mem_areas.begin(); mem_area != mem_areas.end(); ++mem_area) {
			for (auto mem_page = (*mem_area)->pages.begin(); mem_page != (*mem_area)->pages.end(); ++mem_page) {
				page = *mem_page;
				if (page->first_free_block == nullptr){
					if (page->ptr == addr) {
						return;
					}
				}
				else {
					block = page->first_free_block;
					block = go_to_stack_start(block);
					while (block != nullptr) {
						if (block->ptr == addr) {
							return;
						}
						MemoryBlock* next_block = block->next_block;
						if (next_block != nullptr)
							block = next_block;
						else
							break;
					}
				}

			}
		}
		page = nullptr;
		return;
	}

	void break_connections(MemoryBlock* & block, MemoryPage* & page)
	{
		if (page->page_class == "occupied") {
			while (page != nullptr) {
				page->page_class = "free";
				free_pages.push_back(page);
				MemoryPage* newpage = page->next_page;
				page->next_page = nullptr;
				page = newpage;
			}
		}
		else {
			block->is_free = true;
		}
	}

	MemoryBlock* go_to_stack_start(MemoryBlock* block)
	{
		while (block->prev_block != nullptr) {
			MemoryBlock* new_block = (block)->prev_block;
			block = new_block;
		}
		return block;
	}
};

int main()
{
	Allocator* alloc = new Allocator();
	cout << "______________________________________________\n";
	cout << "Allocated memory blocks with size 456 and 4567\n";
	cout << "______________________________________________\n";
	void* ptr = alloc->mem_alloc(456);
	void* ptr2 = alloc->mem_alloc(4567);
	alloc->mem_dump();
	cout << "______________________________________________\n";
	cout << "\nReallocated memory block " << ptr << " to memory block with size 800\n";
	cout << "______________________________________________\n";
	alloc->mem_realloc(ptr, 800);
	alloc->mem_dump();
	cout << "______________________________________________\n";
	cout << "\nFreed memory block " << ptr2 << "\n";
	cout << "______________________________________________\n";
	alloc->mem_free(ptr2);
	alloc->mem_dump();
	system("pause");
}
