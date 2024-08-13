#include "load_scens.hpp"

#include <string>
#include <unordered_map>

bool movingai::traversable(char c)
{
  bool res;
  switch (c) {

  case 'S':
  case 'W':
  case 'T':
  case '@':
  case 'O': // these terrain types are obstacles
    res = false;
    break;
  default: // everything else is traversable
    res = true;
    break;
  }
  return res;
}

movingai::gm_parser::gm_parser(const std::string& filename)
{
	std::fstream mapfs(filename.c_str(), std::fstream::in);
	if(!mapfs.is_open())
	{
		std::cerr << "err; gm_parser::gm_parser "
			"cannot open map file: "<<filename << std::endl;
		exit(1);
	}

	this->parse_header(mapfs);
	this->parse_map(mapfs);
	mapfs.close();
}

movingai::gm_parser::~gm_parser()
{
}

void 
movingai::gm_parser::parse_header(std::fstream& mapfs)
{
	// read header fields
	std::unordered_map<std::string, std::string> contents;
	for(int i=0; i < 3; i++)
	{
		std::string hfield, hvalue;
		mapfs >> hfield;
		if(mapfs.good())
		{
			mapfs >> hvalue;
			if(mapfs.good())
			{
				contents[hfield] = hvalue;
			}
			else
			{
				std::cerr << "err; map load failed. could not read header." << 
					hfield << std::endl;
				exit(1);
			}
		}
		else
		{
			std::cerr << "err;  map load failed. format looks wrong."<<std::endl;
			exit(1);
		}
	}

	this->header_.type_ = contents[std::string("type")];
	if(this->header_.type_.compare("octile") != 0)
	{
		std::cerr << "err; map type " << this->header_.type_ << 
			"is unknown. known types: octile "<<std::endl;;
		exit(1);
	}

	this->header_.height_ = atoi(contents[std::string("height")].c_str());
	if(this->header_.height_ == 0)
	{
		std::cerr << "err; map file specifies invalid height. " << std::endl;
		exit(1);
	}

	this->header_.width_ = atoi(contents[std::string("width")].c_str());
	if(this->header_.width_ == 0)
	{
		std::cerr << "err; map file specifies invalid width. " << std::endl;
		exit(1);
	}

}

void 
movingai::gm_parser::parse_map(std::fstream& mapfs)
{
	std::string hfield;
	mapfs >> hfield;
	if(hfield.compare("map") != 0)
	{
		std::cerr << "err; map load failed. missing 'map' keyword." 
			<< std::endl;
	}

	// read map data
	int index = 0;
	int max_tiles = this->header_.height_*this->header_.width_;
	while(true)
	{
		char c = mapfs.get();	
		if( !mapfs.good() )
		{
			// eof
			break;
		}

		if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
		{
			// skip whitespace
			continue;
		}

		if(index >= max_tiles)
		{
			index++;
			continue;
		}

		this->map_.push_back(c);
		index++;
	}

	if(index != max_tiles)
	{
		std::cerr << "err; expected " << max_tiles
			<< " tiles; read " << index <<" tiles." << std::endl;
		exit(1);
	}
}


movingai::scenario_manager::scenario_manager() : version_(1)
{
}

movingai::scenario_manager::~scenario_manager()
{
	for(unsigned int i=0; i < experiments_.size(); i++)
	{
		delete experiments_[i];
	}
	experiments_.clear();
}

void 
movingai::scenario_manager::load_scenario(const std::string& filelocation)
{
	std::ifstream infile;
	infile.open(filelocation.c_str(),std::ios::in);

	if(!infile.good())
	{
		std::cerr << "err; scenario_manager::load_scenario "
		<< "Invalid scenario file: "<<filelocation << std::endl;
		infile.close();
		exit(1);
	}

	sfile_ = filelocation;


	// Check if a version number is given
	float version=0;
	std::string first;
	infile >> first;
	if(first != "version")
	{
		version = 0.0;
		infile.seekg(0,std::ios::beg);
	}

	infile >> version;
	if(version == 1.0 || version == 0)
	{
		load_v1_scenario(infile);
	}
	else
	{
		std::cerr << "err; scenario_manager::load_scenario "
			<< " scenario has invalid version number. \n";
		infile.close();
		exit(1);
	}

	infile.close();
}

// V1.0 is the version officially supported by HOG
void 
movingai::scenario_manager::load_v1_scenario(std::ifstream& infile)
{
	int sizeX = 0, sizeY = 0; 
	int bucket;
	std::string map;  
	int xs, ys, xg, yg;
	std::string dist;

	while(infile>>bucket>>map>>sizeX>>sizeY>>xs>>ys>>xg>>yg>>dist)
	{
		double dbl_dist = strtod(dist.c_str(),0);
		experiments_.push_back(
				new experiment(xs,ys,xg,yg,sizeX,sizeY,dbl_dist,map));

		int precision = 0;
		if(dist.find(".") != std::string::npos)
		{
			precision = dist.size() - (dist.find(".")+1);
		}
		experiments_.back()->set_precision(precision);
	}
}
