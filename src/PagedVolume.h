#pragma once

#include <stdio.h>
#include <string.h>

template <class T> class PagedVolume
	{
	public:
		inline PagedVolume(unsigned int sizeX, unsigned int sizeY, unsigned int sizeZ)
		{
			this->sizeX = sizeX;
			this->sizeY = sizeY;
			this->sizeZ = sizeZ;

			this->alloc();
		}

		inline ~PagedVolume()
		{
			this->free();
		}

		inline T** getData()
		{
			return data;
		}

		inline T getDataValue(unsigned int x, unsigned int y, unsigned int z)
		{
			return data[z][sizeX * y + x];
		}

		inline void setDataValue(T value, unsigned int x, unsigned int y, unsigned int z)
		{
			data[z][sizeX * y + x] = value;
		}

		inline T* getDataPage(unsigned int page)
		{
			return data[page];
		}

		inline unsigned int getSizeX()
		{
			return sizeX;
		}

		inline unsigned int getSizeY()
		{
			return sizeY;
		}

		inline unsigned int getSizeZ()
		{
			return sizeZ;
		}

		inline unsigned int getPageCount()
		{
			return sizeZ;
		}

		inline unsigned int getPageSize()
		{
			return sizeX * sizeY;
		}

		inline void free()
		{
			for (unsigned int i = 0; i < this->getPageCount(); i++)
			{
				delete[] data[i];
			}
			delete[] data;
		}

		inline void alloc()
		{
			unsigned int pageCount = this->getPageCount();
			unsigned int pageSize = this->getPageSize();

			try
			{
				data = new T * [pageCount];
				for (unsigned int i = 0; i < pageCount; i++)
				{
					printf("%d\n", i);
					data[i] = new T[pageSize];
					memset(data[i], 0, pageSize);
				}
			}
			catch (...)
			{
				printf("PagedVolume::alloc() : insufficient memory");
			}
		}




	private:
		T **data;
		unsigned int sizeX;
		unsigned int sizeY;
		unsigned int sizeZ;

};

