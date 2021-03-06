#include <utility>
#include <SFML/Graphics.hpp>
#include <iostream>

#include "gui.hpp"
#include "menu.hpp"

gui::Menu::Menu(const sf::Vector2u& alignment, const sf::Vector2u& entrySize,
				const gui::Font& font, const sf::Color& backgroundCol,
				const sf::Color& textCol) :
	mCurrentPage(0),
	mAlignment(alignment),
	mEntrySize(entrySize),
	mFont(&font),
	mSelectedEntry(0),
	mSelectorCharacter(' '),
	mBackgroundCol(backgroundCol),
	mTextCol(textCol),
	mTrim(false)
{
	formatEntries();
	generateGeometry();
}

void gui::Menu::formatEntries()
{
	// First split each entry into lines which fit within the desired bounds
	std::vector<std::vector<std::string>> alignedEntries;
	for(auto entry : mEntries)
	{
		if(mTrim)
			alignedEntries.push_back({entry.first.substr(0, mEntrySize.x)});
		else
			alignedEntries.push_back(gui::alignString(entry.first, mEntrySize.x));
	}

	// If laid out from left to right and top to bottom, then the
	// quotient of (number of entries) / (entries per row) will give
	// the number of completed rows, and the remainder will give the
	// number of entries on the partially completed row.
	unsigned int numRows = mEntries.size() / mAlignment.x;
	unsigned int partialRow = mEntries.size() % mAlignment.x;
	// We now concatenate the first mEntrySize.y lines of each
	// group of mAlignment.x entries. As an example if mAlignment is (3,1)
	// and mEntrySize is (5, 2) we will transform
	// {
	// 		{"opt1"},
	// 		{"opti2","line2"},
	// 		{"op3", "line2", "line3"}
	// }
	// to
	// {
	// 		{" opt1  ", "       "},
	// 		{" opti2 ", " line2 "},
	// 		{" op3   ", " line2 ", " line3 "}
	// }
	// and then to
	// {
	// 		{" opt1   opti2  op3   "},
	// 		{"        line2  line2 "}
	// }
	// Note that we will also add the correct padding, and will discard any
	// text outside of the bounds (if each entry were a messagebox, they would
	// be on another page).
	for(size_t i = 0; i < alignedEntries.size(); ++i)
	{
		// We need an index variable to check for selected elements
		auto& entry = alignedEntries[i];

		// First pad the existing rows
		for(size_t row = 0; row < entry.size(); ++row)
		{
			// Far left and right padding is 1, internal padding is 2,
			// which is equivalent to each entry being surrounded by 1
			// space. Each entry should also have additional right padding
			// so that it's the same length as mEntrySize.x
			if(i == mSelectedEntry && row == 0)
				entry[row] = std::string(1, mSelectorCharacter) + entry[row];
			else
				entry[row] = " " + entry[row];
			entry[row] += std::string(2 + mEntrySize.x-entry[row].size(), ' ');
		}
		// Now add additional rows made of whitespace, if necessary
		for(size_t row = entry.size(); row < mEntrySize.y; ++row)
		{
			entry.push_back(std::string(mEntrySize.x + 2, ' '));
		}
	}
	// Now each entry is padded, they can be combined
	mAlignedLines.clear();
	// Note row is not a line, but rather a row of entries
	// which may span multiple lines
	for(size_t row = 0; row < numRows; ++row)
	{
		for(size_t line = 0; line < mEntrySize.y; ++line)
		{
			std::string alignedLine;
			for(size_t entry = 0; entry < mAlignment.x; ++entry)
			{
				alignedLine += alignedEntries[row * mAlignment.x + entry][line];
			}
			mAlignedLines.push_back(alignedLine);
		}
	}
	// Now do the last partial row, if there is one
	if(partialRow > 0)
	{
		for(size_t line = 0; line < mEntrySize.y; ++line)
		{
			std::string alignedLine;
			for(size_t entry = 0; entry < partialRow; ++entry)
			{
				alignedLine +=
					alignedEntries[numRows * mAlignment.x + entry][line];
			}
			std::string padding((mAlignment.x-partialRow) * (2+mEntrySize.x), ' ');
			if(mSelectedEntry / mAlignment.x == numRows &&
				mSelectedEntry % mAlignment.x >= partialRow &&
				line == 0)
			{
				padding[((mSelectedEntry-1) % mAlignment.x) * (mEntrySize.x+2)] = '*';
			}
			alignedLine += padding;
			mAlignedLines.push_back(alignedLine);
		}
	}
}

void gui::Menu::generatePage(size_t start, size_t end)
{
	// Width and height of final text, without borders
	unsigned int width = mAlignment.x * (mEntrySize.x + 2);
	unsigned int height = mAlignment.y * mEntrySize.y;

	// Add top border
	std::string textStr = gui::Border::genTop(width+2);

	// Add entry lines, up to the maximum number allowable
	size_t bound = end;
	if(end-start > height) bound = start + height;
	if(bound > mAlignedLines.size()) bound = mAlignedLines.size();
	for(size_t i = start; i < bound; ++i)
	{
		textStr += gui::Border::surround(mAlignedLines[i]) + "\n";
	}

	// Add padding lines, up to the indented height
	std::string padding(width, ' ');
	for(size_t i = bound; i < start+height; ++i)
	{
		// Add the selector character if necessary
		if(mSelectedEntry / mAlignment.x == i / mEntrySize.y &&
			i % mEntrySize.y == 0)
		{
			size_t loc = (mSelectedEntry % mAlignment.x) * (mEntrySize.x+2);
			padding[loc] = '*';
			textStr += gui::Border::surround(padding) + "\n";
			padding[loc] = ' ';
		}
		else
		{
			textStr += gui::Border::surround(padding) + "\n";
		}
	}

	// Add bottom border
	textStr += gui::Border::genBottom(width+2);

	// Finally we can create the gui::Text and add it to the pages
	mPages.push_back(gui::Text(textStr, *mFont));
	mPages.back().setColor(mTextCol);
	mPages.back().setBackgroundColor(mBackgroundCol);
}

void gui::Menu::generateGeometry()
{
	size_t lines = mAlignment.y * mEntrySize.y;
	mPages.clear();
	size_t numPages = 1 + mAlignedLines.size() / lines;
	if(mAlignedLines.size() % lines == 0 && mAlignedLines.size() > 0)
		numPages -= 1;

	for(size_t i = 0; i < numPages; ++i)
	{
		generatePage(i*lines, (i+1)*lines);
	}
}

void gui::Menu::select(size_t index, unsigned char selector)
{
	mSelectedEntry = index;
	mSelectorCharacter = selector;
	formatEntries();
	generateGeometry();
}

void gui::Menu::activate(void* ptr)
{
	if(mSelectedEntry < mEntries.size() && mEntries.at(mSelectedEntry).second)
		mEntries.at(mSelectedEntry).second(ptr, mSelectedEntry);
}

void gui::Menu::addEntry(const std::string& entry, void (*callback)(void*, size_t))
{
	mEntries.push_back(std::make_pair(entry, callback));
	formatEntries();
	generateGeometry();
}

void gui::Menu::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	states.transform *= getTransform();
	mPages.at(mCurrentPage).draw(target, states);
}

const sf::Color& gui::Menu::getBackgroundColor() const
{
	return mBackgroundCol;
}

const sf::Color& gui::Menu::getColor() const
{
	return mTextCol;
}

void gui::Menu::setBackgroundColor(const sf::Color& backgroundCol)
{
	mBackgroundCol = backgroundCol;
	for(auto& page : mPages)
		page.setBackgroundColor(backgroundCol);
}

void gui::Menu::setColor(const sf::Color& textCol)
{
	mTextCol = textCol;
	for(auto& page : mPages)
		page.setColor(textCol);
}

sf::Vector2u gui::Menu::getSize() const
{
	return sf::Vector2u(
						(mEntrySize.x + 2) * mAlignment.x + 2,
						(mEntrySize.y    ) * mAlignment.y + 2);
}

void gui::Menu::navigate(gui::Direction dir, gui::NavigationMode xMode,
						 gui::NavigationMode yMode)
{
	size_t entriesOnPrevPage = mCurrentPage*mAlignment.x*mAlignment.y;
	sf::Vector2u pos((mSelectedEntry-entriesOnPrevPage) % mAlignment.x,
					 (mSelectedEntry-entriesOnPrevPage) / mAlignment.x);

	switch(dir)
	{
		case gui::Direction::UP:
		// Start of vertical column
		if(pos.y == 0)
		{
			if(yMode == gui::NavigationMode::LOOP)
			{
				pos.y = mAlignment.y - 1;
			}
			else if(yMode == gui::NavigationMode::ADVANCE)
			{
				pos.y = mAlignment.y - 1;
				// Remember that pos.x is an *unsigned* integer, so recklessly
				// decrementing is a no-go if pos.x might be 0
				if(pos.x == 0) pos.x = mAlignment.x - 1;
				else --pos.x;
			}
			else if(yMode == gui::NavigationMode::PAGE)
			{
				if(mCurrentPage == 0) mCurrentPage = mPages.size();
				--mCurrentPage;
				pos.y = mAlignment.y-1;
			}
		}
		else
			--pos.y;
		break;

		case gui::Direction::DOWN:
		// End of vertical column
		if(pos.y == mAlignment.y-1)
		{
			if(yMode == gui::NavigationMode::LOOP)
			{
				pos.y = 0;
			}
			else if(yMode == gui::NavigationMode::ADVANCE)
			{
				pos.y = 0;
				if(pos.x >= mAlignment.x-1) pos.x = 0;
				else ++pos.x;
			}
			else if(yMode == gui::NavigationMode::PAGE)
			{
				++mCurrentPage;
				pos.y = 0;
			}
		}
		else
			++pos.y;
		break;

		case gui::Direction::RIGHT:
		// End of horizontal row
		if(pos.x == mAlignment.x - 1)
		{
			if(xMode == gui::NavigationMode::LOOP)
			{
				pos.x = 0;
			}
			else if(xMode == gui::NavigationMode::ADVANCE)
			{
				pos.x = 0;
				if(pos.y == mAlignment.y - 1) pos.y = 0;
				else ++pos.y;
			}
			else if(xMode == gui::NavigationMode::PAGE)
			{
				++mCurrentPage;
				pos.x = 0;
			}
		}
		else
			++pos.x;
		break;

		case gui::Direction::LEFT:
		// Start of horizontal row
		if(pos.x == 0)
		{
			if(xMode == gui::NavigationMode::LOOP)
			{
				pos.x = mAlignment.x - 1;
			}
			else if(xMode == gui::NavigationMode::ADVANCE)
			{
				pos.x = mAlignment.x - 1;
				if(pos.y == 0) pos.y = mAlignment.y - 1;
				else --pos.y;
			}
			else if(xMode == gui::NavigationMode::PAGE)
			{
				if(mCurrentPage == 0) mCurrentPage = mPages.size();
				--mCurrentPage;
				pos.x = mAlignment.x - 1;
			}
		}
		else
			--pos.x;
		break;
	}

	mCurrentPage %= mPages.size();
	size_t index = pos.y * mAlignment.x + pos.x + mCurrentPage*mAlignment.x*mAlignment.y;
	select(index, mSelectorCharacter);
}

size_t gui::Menu::getPage() const
{
	return mCurrentPage;
}
size_t gui::Menu::numPages() const
{
	return mPages.size();
}
void gui::Menu::setPage(size_t page)
{
	mCurrentPage = page % mPages.size();
}

void gui::Menu::setTrim(bool trim)
{
	mTrim = trim;
}

size_t gui::Menu::getSelected() const
{
	return mSelectedEntry;
}
