#include <DK.h>

void* operator new (std::size_t size)
{
	if (size == 0)
		size = 1;
	void* p = nullptr;
	while ((p = DKFoundation::DKMalloc(size)) == nullptr)
	{
		std::new_handler handler = std::get_new_handler();
		if (handler)
			handler();
		else
			throw std::bad_alloc();
	}
	return p;
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
	void* p = nullptr;
	try
	{
		p = ::operator new(size);
	}
	catch (...) {}
	return p;
}

void* operator new[](std::size_t size)
{
	return ::operator new(size);
}

void* operator new[](size_t size, const std::nothrow_t& tag) noexcept
{
	return ::operator new(size, tag);
}

void operator delete (void* ptr) noexcept
{
	if (ptr)
		DKFoundation::DKFree(ptr);
}

void operator delete (void* ptr, const std::nothrow_t&) noexcept
{
	::operator delete(ptr);
}

void operator delete[](void* ptr) noexcept
{
	::operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
	::operator delete(ptr);
}
