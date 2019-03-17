#include <iostream>
#include<complex>
#include<valarray> //only needed for slice()
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <SFML/Audio.hpp>
//#include <vector> is already included from SFML
using namespace std;

typedef complex<double> Complex;
typedef valarray<Complex> CArray;//can be a vector but slice needs to be implemented
vector<vector<long>> fftMatrixData; //2D array stores precomputed fft
int fftMatrixDataMaxAmplitude = 0;
const double PI = 3.141592653589793238460;

//std::vector<sf::Int16> entireSong;



CArray entireSong;
CArray currentSample;
sf::Uint64 last = 0; //keeps track of the last element in entireSong
int fftProgress = 0; // tracks progress of fft on samples
float songDuration = 0;
sf::Uint64 totalSamples;
int sampleRate = 0;
const short sampleDivisor = 128;//a journal said 250 was good though , 82 what fraction of a second should be analyzed//https://www.mathsisfun.com/numbers/factors-all-tool.html
const int sampleRateConst = 44100 / sampleDivisor;//SFML only supports 41,000hz, If this number is not correct then The audio visualizer will be out of sync.
int mytime = 0;//used as an index to choose which set of data to show based on time
int offset = 0;
enum plotStyles { points, lines, bars };
plotStyles plottingStyle = bars;
bool plotLog = false;
bool ploted = false;
void Draw() {
	//if (ploted)return;
	glClear(GL_COLOR_BUFFER_BIT);
	glPointSize(2.0f);
	glColor3f(0.0f, 1.0f, 1.0f);
	//glRect(x1, y1, x2, y2)

	int min, max, prevHeight;
	min = 99999;
	max = 0;
	prevHeight = 0;

	if (mytime < songDuration*sampleDivisor && mytime>0)
	{
		if (plottingStyle == plotStyles::points)
			glBegin(GL_POINTS);
		else if (plottingStyle == plotStyles::lines)
			glBegin(GL_LINE_STRIP);
		else if (plottingStyle == plotStyles::bars)
		{
			//glBegin(GL_POLYGON);//4 points needed or glRect(x1, y1, x2, y2) bottom left then bottom right cant be between glbegin end glEnd

			for (int i = 0; i < sampleRateConst; i++)
			{
				/*get min max
				if (fftMatrixData[mytime][i] > max)
				max = fftMatrixData[mytime][i];
				if (fftMatrixData[mytime][i] < min)
				min = fftMatrixData[mytime][i];
				*/
				if (plotLog)
				{
					//Log scale Bars ploting style counter clockwise winding
					if (i % 2 == 0)
					{
						glBegin(GL_POLYGON);
						glVertex2f(((float)((float)(log10(i) * 41000) / sampleRateConst) / ((float)(log10(sampleRateConst) * 41000) / sampleRateConst)) * 2 - 1, 0);//even
					}
					else
					{
						//odd
						glVertex2f(((float)((float)(log10(i) * 41000) / sampleRateConst) / ((float)(log10(sampleRateConst) * 41000) / sampleRateConst)) * 2 - 1, 0);//Bottom Right
						glVertex2f(((float)((float)(log10(i) * 41000) / sampleRateConst) / ((float)(log10(sampleRateConst) * 41000) / sampleRateConst)) * 2 - 1, (float)((float)prevHeight + fftMatrixData[mytime][i] / 2) / fftMatrixDataMaxAmplitude);//Top Right
						glVertex2f(((float)((float)(log10(i - 1) * 41000) / sampleRateConst) / ((float)(log10(sampleRateConst) * 41000) / sampleRateConst)) * 2 - 1, (float)((float)prevHeight + fftMatrixData[mytime][i] / 2) / fftMatrixDataMaxAmplitude);//Top Left
						glEnd();//close Bar
					}
				}
				else
				{
					//Linear Bars ploting style
					if (i % 2 == 0) {
						glBegin(GL_POLYGON);
						glVertex2f(((float)((float)(i * 41000) / sampleRateConst) / 41000) * 2 - 1, 0);//even
					}
					else
					{
						glVertex2f(((float)((float)(i * 41000) / sampleRateConst) / 41000) * 2 - 1, 0);//Bottom Right
						glVertex2f(((float)((float)(i * 41000) / sampleRateConst) / 41000) * 2 - 1, (float)((float)prevHeight + fftMatrixData[mytime][i] / 2) / fftMatrixDataMaxAmplitude);//Top Right
						glVertex2f(((float)((float)((i - 1) * 41000) / sampleRateConst) / 41000) * 2 - 1, (float)((float)prevHeight + fftMatrixData[mytime][i] / 2) / fftMatrixDataMaxAmplitude);//Top Left
						glEnd();//close Bar
					}
				}
				prevHeight = fftMatrixData[mytime][i];
			}

			//std::cout << "Min is " << min << " Max is " << max << " Matsize " << fftMatrixData.size() << "\r" << flush;
			ploted = true;
			return;
		}

		//start plot

		for (int i = 0; i < sampleRateConst; i++)
		{
			/*
			if (fftMatrixData[mytime][i] > max) {
			max = fftMatrixData[mytime][i];
			}
			if (fftMatrixData[mytime][i] < min)
			{
			min = fftMatrixData[mytime][i];
			}
			*/
			if (plotLog)
			{
				//plots lines and points in log scale
				glVertex2f(((float)((float)(log10(i) * 41000) / sampleRateConst) / ((float)(log10(sampleRateConst) * 41000) / sampleRateConst)) * 2 - 1, (float)fftMatrixData[mytime][i] / fftMatrixDataMaxAmplitude);
			}
			else
				glVertex2f(((float)((float)(i * 41000) / sampleRateConst) / 41000) * 2 - 1, (float)fftMatrixData[mytime][i] / fftMatrixDataMaxAmplitude);

			prevHeight = fftMatrixData[mytime][i];
		}
		//std::cout << "Min is " << min << " Max is " << max << " Matsize " << fftMatrixData.size() << "\r" << flush;
		//min 24 max 84739437 matsize 33
		ploted = true;
		glEnd();
	}
}

void Update() {
	std::cout << mytime << '\r' << flush;
}

void fft(CArray &x)
{
	const int N = x.size();
	if (N <= 1) return;

	CArray even = x[slice(0, N / 2, 2)];
	CArray  odd = x[slice(1, N / 2, 2)];

	fft(even);
	fft(odd);

	for (int k = 0; k < N / 2; k++)
	{
		Complex t = polar(1.0, -2 * PI * k / N) * odd[k];
		x[k] = even[k] + t;
		x[k + N / 2] = even[k] - t;
	}
	if (N>fftProgress)
	{
		fftProgress = N;
		cout << "Progress: " << (short)(((float)N / (float)totalSamples)*50.0f) + 50.f << "%" << '\r' << flush;
	}

}

int main() {

#pragma region WindowGLEWSetup
	sf::ContextSettings settings;

	settings.majorVersion = 3;
	settings.minorVersion = 3;
	settings.depthBits = 24;

	sf::RenderWindow window(sf::VideoMode(800, 600), "Window Title Here!", sf::Style::Default, settings);
	window.setActive(true);

	glewExperimental = true;
	GLenum result = glewInit();
	if (result != GLEW_OK)
	{
		std::cout << "Glew failed to Initialize" << glewGetErrorString(result) << std::endl;
	}
#pragma endregion WindowGLEWSetup 

	sf::InputSoundFile file;
	string filenameF = "Song.ogg";//144Hz.wav 400Hz.wav 500Hz.wav 10000Hz.wav Song.ogg
	if (!file.openFromFile(filenameF))
		std::cout << "could not open file";
	sampleRate = file.getSampleRate();
	totalSamples = file.getSampleCount();
	songDuration = file.getDuration().asSeconds();
	// Print the sound attributes
	std::cout << "Name: " << filenameF << std::endl;
	std::cout << "duration: " << songDuration << std::endl;
	std::cout << "channels: " << file.getChannelCount() << std::endl;
	std::cout << "sample rate: " << sampleRate << std::endl;
	std::cout << "sample count: " << totalSamples << std::endl;

	sf::Int16 samples[sampleRateConst];

	//fftMatrixData.resize(totalSamples / 44100);//dosent matter if odd || this causes eeror cause push back places in wrong location
	vector<long> fftMatrixRow(sampleRateConst, 0);
	//entireSong.resize(totalSamples);//valarray cant change size so setting the total length to 0 is good
	currentSample.resize(sampleRateConst);
	sf::Uint64 count;
	sf::SoundBuffer buffer;
	sf::Sound sound;
	//bool first = true;
	sf::Uint64 howmanysamplesread = 0;
	do
	{
		//first = true;
		count = file.read(samples, sampleRateConst);//how much audio to read default 1024 //41000 samples on a 41000 sample rate song = 1 seconde//
		howmanysamplesread += count;
		for (int i = 0; i < count; i++)
		{
			if (i + last < totalSamples) {
				//entireSong[i + last] = samples[i];//dont need this
				//fftMatrixRow[i]= samples[i];
				currentSample[i] = samples[i];
			}
			else {
				currentSample[i] = 0;
			}

			//entireSong.push_back(samples[i]);
		}
		last += count;
		fft(currentSample);
		int amplitude = 0;
		for (unsigned int i = 0; i < currentSample.size(); i++)
		{
			amplitude = abs(currentSample[i]);
			fftMatrixRow[i] = amplitude;
			if (fftMatrixDataMaxAmplitude < amplitude)
				fftMatrixDataMaxAmplitude = amplitude;
		}
		fftMatrixData.push_back(fftMatrixRow);
		//cout <<"Progress: "<< (short)(((float)entireSong.size()/(float)totalSamples)*100.0f)<<"%"<<endl;
		std::cout << "Progress: " << (short)(((float)last / (float)totalSamples)*50.0f) << "%" << '\r' << flush;

		/*
		buffer.loadFromSamples(&samples[0], count, 2, 44100);
		sound.setBuffer(buffer);
		sound.play();
		while (sound.getStatus() == sf::SoundSource::Status::Playing) {
		if (first == true)
		{
		for (int i = 0; i < 44100; i++) {
		entireSong.push_back(samples[i]);
		}
		first = false;
		}
		}
		*/

	} while (count > 0);
	/* load and play buffer
	buffer.loadFromSamples(&entireSong[0], entireSong.size(), 2, 44100);
	sound.setBuffer(buffer);
	sound.play();
	*/
	std::cout << "samples readfrom file " << howmanysamplesread << endl;//377344
	fftMatrixDataMaxAmplitude = fftMatrixDataMaxAmplitude / 4; // zoom 
	sf::Music music;
	if (!music.openFromFile(filenameF))
		std::cout << "errr";

	//fft(entireSong);




	glClearColor(0, 0, 0, 1);//RGB bLACK background


	music.play();
	sf::Clock clock; // starts the clock
	sf::Time elapsedTime = clock.getElapsedTime();
	bool mute = false;
	while (window.isOpen())
	{
		sf::Event ev;
		while (window.pollEvent(ev))
		{
			switch (ev.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::Resized:
				glViewport(0, 0, ev.size.width, ev.size.height);
				break;
			case sf::Event::KeyPressed:
				switch (ev.key.code) {
				case sf::Keyboard::R:
					//restart song
					clock.restart();
					music.stop();
					music.play();
					break;
				case sf::Keyboard::M:
					//Mute song
					mute = !mute;
					music.setVolume(100 * mute);
					break;
				case sf::Keyboard::P:
					//Pause Song also need to pause clock
					/*

					if(music.getStatus() == sf::Music::Status::Playing)
					music.pause();
					else
					music.play();
					*/
					break;
				case sf::Keyboard::L:
					//change plotting style
					plottingStyle = plotStyles::lines;
					break;
				case sf::Keyboard::I:
					//change plotting style
					plottingStyle = plotStyles::points;
					break;
				case sf::Keyboard::B:
					//change plotting style
					plottingStyle = plotStyles::bars;
					break;
				case sf::Keyboard::O:
					//change plotting style
					if (plotLog)
						plotLog = false;
					else
						plotLog = true;
					break;
				case sf::Keyboard::Left:
					offset -= 1;
					break;
				case sf::Keyboard::Right:
					offset += 1;
					break;
				}
				break;
			}
		}

		//if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) {  }


		//Update();

		elapsedTime = clock.getElapsedTime();
		mytime = (int)(elapsedTime.asSeconds() * sampleDivisor) + offset;
		Draw();
		//deltaTime = clock.restart();//restarts and returns deltaTime
		window.display();
	}

	return 0;
}