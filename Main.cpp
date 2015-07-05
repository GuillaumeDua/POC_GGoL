#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <random>

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Window.hpp>

namespace Config
{
	static const std::pair<size_t, size_t>	ScreenSize = std::make_pair(800, 800);
	static const size_t	MaxCellLifeLength = 10;
	static const size_t MinParentsToBorn = 2;
	static const size_t MinCycleToLiveToReproduce = 2;

	sf::Color CycleToColorValue(const size_t cycle)
	{
		const sf::Uint8 val = (cycle * 255) / MaxCellLifeLength;
		return {val,val,val};
	};
}
namespace RNG
{
	std::default_random_engine			generator;
	std::uniform_int_distribution<int>	distribution(2, Config::MaxCellLifeLength);

	inline const int	Get(void)
	{
		return distribution(generator);
	}
}

template <typename T>
std::string	LexicalCast(const T & value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

struct IEntity
{
	virtual void Draw(sf::RenderWindow &) = 0;
	virtual void Behave(void) = 0;
};
struct Cell : protected IEntity
{
	static const float	size;

	Cell(const sf::Vector2f & pos)
		: _shape(sf::Vector2f(size,size))
		, _pos(pos)
	{
		_shape.setFillColor(sf::Color::Black);
		_shape.setPosition(_pos);
	}

	void	Born(void)
	{
		_remainingCyclesToLive = RNG::Get();
		_shape.setFillColor(Config::CycleToColorValue(_remainingCyclesToLive));
	}
	void	Die(void)
	{
		_remainingCyclesToLive = 0;
		_shape.setFillColor(sf::Color::Black);
	}

	void	Draw(sf::RenderWindow & rw)
	{
		rw.draw(_shape);
	}
	void	Behave(void)
	{
		if (!_remainingCyclesToLive)
		{
			const size_t aliveNearbyCells = std::count_if(_adjacentCells.cbegin(), _adjacentCells.cend(), [](const Cell * cell) -> bool { return cell->_remainingCyclesToLive > Config::MinCycleToLiveToReproduce; });
			if (aliveNearbyCells > Config::MinParentsToBorn)
				this->_nextStatus = NextStatus::WillBorn;
		}
		else
		{
			if (--_remainingCyclesToLive == 0)
				this->_nextStatus = NextStatus::WillDie;
			else
			{
				const sf::Color & currentColor = _shape.getFillColor();
				_shape.setFillColor(Config::CycleToColorValue(_remainingCyclesToLive));
			}
		}
	}
	void	ApplyStatus(void)
	{
		switch (_nextStatus)
		{
		case WillDie:
			Die();
			break;
		case WillBorn:
			Born();
			break;
		case NextStatus::Same:
		default:
			break;
		}
		this->_nextStatus = NextStatus::Same;
	}

	void	HighLightAdjacentCells(void)
	{
		std::cout << _adjacentCells.size() << std::endl;
		std::for_each(_adjacentCells.begin(), _adjacentCells.end(), [](Cell * cell){ cell->_shape.setFillColor(sf::Color::Green); });;
	}

	std::vector<Cell*>	_adjacentCells;	// [3->8]



//protected:
	enum NextStatus
	{
		Same
		, WillDie
		, WillBorn
	}	_nextStatus = NextStatus::Same;

	size_t				_remainingCyclesToLive = 0;
	sf::RectangleShape	_shape;
	sf::Vector2f		_pos;
	sf::Text			_text;
};

const float		Cell::size = 5;

struct	Grid : public IEntity
{
	Grid(const size_t size)
		: _size(size, size)
	{
		for (size_t y_it = 0; y_it < size; ++y_it)
			for (size_t x_it = 0; x_it < size; ++x_it)
				_cells.emplace_back(std::move(Cell(sf::Vector2f(static_cast<float>(x_it * Cell::size), static_cast<float>(y_it * Cell::size)))));

		int cellId = 0;
		for (size_t y_it = 0; y_it < size; ++y_it)
			for (size_t x_it = 0; x_it < size; ++x_it)
			{
				long long N = cellId - size;
				long long S = cellId + size;
				long long E = cellId + 1;
				long long W = cellId - 1;
				long long NE = cellId - size + 1;
				long long NW = cellId - size - 1;
				long long SE = cellId + size + 1;
				long long SW = cellId + size - 1;

				if (y_it != 0)
				{
					_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(N)));		// N
					if (x_it != (size - 1))
						_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(NE)));	// NE
					if (x_it != 0)
						_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(NW)));	// NW
				}
				if (y_it != (size - 1))
				{
					_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(S)));		// S
					if (x_it != (size - 1))
						_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(SE)));	// SE
					if (x_it != 0)
						_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(SW)));	// SW
				}
				if (x_it != (size - 1))
					_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(E)));		// E
				if (x_it != 0)
					_cells.at(cellId)._adjacentCells.push_back(&(_cells.at(W)));		// W

				cellId++;
			}
	}

	void	Draw(sf::RenderWindow & rw)
	{
		for (auto & elem : _cells)
			elem.Draw(rw);
	}
	void	Behave(void)
	{
		for (auto & elem : _cells)
			elem.Behave();
		std::for_each(_cells.begin(), _cells.end(), [&](Cell & cell){ cell.ApplyStatus(); });
	}

	inline Cell &	GetCellFromCoord(const std::pair<size_t, size_t> & coord)
	{
		static const size_t cellSizeX = Config::ScreenSize.first / _size.first;
		static const size_t cellSizeY = Config::ScreenSize.second / _size.second;
		return _cells.at((coord.first / cellSizeX) + (coord.second / cellSizeY * _size.first));
	}

	std::vector<Cell>				_cells;
	const std::pair<size_t, size_t>	_size;
};

static	void Type1Cell_initialize(Cell & cell)
{
	cell.Born();
	cell._shape.setFillColor(sf::Color::Blue);
	for_each(cell._adjacentCells.begin(), cell._adjacentCells.end(), [](Cell * cell){ cell->Born(); });
}
static	void Type2Cell_initialize(Cell & cell)
{
	cell.Born();
	cell._shape.setFillColor(sf::Color::Blue);
	for_each(cell._adjacentCells.begin(), cell._adjacentCells.end(), [](Cell * cell){ cell->Born(); Type1Cell_initialize(*cell); });
}
static	void Type3Cell_initialize(Cell & cell)
{
	cell.Born();
	cell._shape.setFillColor(sf::Color::Blue);
	for_each(cell._adjacentCells.begin(), cell._adjacentCells.end(), [](Cell * cell){ cell->Born(); Type2Cell_initialize(*cell); });
}

int	main(int ac, char *av[])
{
	sf::Font font;
	font.loadFromFile("font.ttf");
	sf::Text text("__INIT__", font);
	text.setCharacterSize(25);
	text.setStyle(sf::Text::Bold);
	text.setColor(sf::Color::Red);
	text.setPosition(400, 0);

	sf::RenderWindow window(sf::VideoMode(Config::ScreenSize.first, Config::ScreenSize.second), "GGoL_Poc");
	Grid grid(160);

	for (size_t it = 0; it < 10; ++it)
		Type3Cell_initialize(grid._cells.at(9000 + it * 100));

	size_t	cycles = 0;
	bool	isMouseButtonPressing = false;
	while (window.isOpen())
	{
		const size_t alivedCells = std::count_if(grid._cells.cbegin(), grid._cells.cend(), [](const Cell & cell) -> bool { return cell._remainingCyclesToLive != 0; });

		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::MouseButtonPressed)
			{
				isMouseButtonPressing = true;
				Type1Cell_initialize(grid.GetCellFromCoord(std::make_pair(event.mouseButton.x, event.mouseButton.y)));
			}
			if (event.type == sf::Event::MouseButtonReleased)
				isMouseButtonPressing = false;
		}

		if (isMouseButtonPressing)
		{
			Type1Cell_initialize(grid.GetCellFromCoord(std::make_pair(event.mouseButton.button, event.mouseButton.x)));
			std::cout << event.mouseButton.x << ", " << event.mouseButton.y << ", " << event.mouseButton.button << std::endl;
		}


		grid.Behave();

		{
			std::ostringstream oss;
			oss << "cycles : [" << std::setw(5) << cycles << "], cells : [" << std::setw(4) << alivedCells << ']' << std::endl;
			text.setString(oss.str());
		}
		window.clear();
		grid.Draw(window);
		window.draw(text);
		window.display();

		if (alivedCells == 0)
			break;
		sf::sleep(sf::milliseconds(100));
		cycles++;
	}

	system("pause");
	return 0;
}