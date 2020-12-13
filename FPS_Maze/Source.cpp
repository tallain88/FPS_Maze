#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <algorithm>


int const k_MaxScreenWidth = 120;
int const k_MaxScreenHeight = 40;

int const k_MaxMapWidth = 16;
int const k_MaxMapHeight = 16;

//player position
float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerRotation = 0.0f;

//field of view
float fFOV = 3.14159 / 4;

//set depth
float fDepth = 16.0f;


int main()
{
	// Create Screen Buffer
	wchar_t* screen = new wchar_t[k_MaxScreenHeight * k_MaxScreenWidth];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// Map
	std::wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"####...........#";
	map += L"#..#...........#";
	map += L"#..#...........#";
	map += L"#..........#...#";
	map += L"#..........#...#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"########.......#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";


	// Start and end times
	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	while (1)
	{
		//calculate game time
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		//Controls
		if (GetAsyncKeyState((unsigned short) 'A') & 0X8000)
		{
			fPlayerRotation -= (0.8f) * fElapsedTime;
		}
		
		if (GetAsyncKeyState((unsigned short) 'D') & 0x8000)
		{
			fPlayerRotation += (0.8f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerRotation) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerRotation) * 5.0f * fElapsedTime;

			//check for collisions
			if (map[(int)fPlayerY * k_MaxMapWidth + (int)fPlayerX] == '#')
			{
				//undo last move forward
				fPlayerX -= sinf(fPlayerRotation) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerRotation) * 5.0f * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerRotation) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerRotation) * 5.0f * fElapsedTime;

			//check for collisions
			if (map[(int)fPlayerY * k_MaxMapWidth + (int)fPlayerX] == '#')
			{
				//undo last move backwards
				fPlayerX += sinf(fPlayerRotation) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerRotation) * 5.0f * fElapsedTime;
			}
		}

		//screen width ray scan
		for (int x = 0; x < k_MaxScreenWidth; x++)
		{
			// For each column on the screen in FOV, get distanct to that column
			// divide FOV by two (get player focus) then divide that by screen width to get total visible
			// columns 
			float fRayAngle = (fPlayerRotation - fFOV / 2.0f) + ((float)x / (float)k_MaxScreenWidth) * fFOV;
			float fDistanceToWall = 0.0;
			bool bHasHitWall = false;
			bool bHasHitBoundry = false; //boundry = 'edge'

			//vector of player focus
			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			// Loop to check if ray has hit wall and that it is less than our max depth
			while (!bHasHitWall && fDistanceToWall < fDepth)
			{
				//increment ray to wall
				fDistanceToWall += 0.1f;

				int testX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int testY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds (height and width)
				if (testX < 0 || testX >= k_MaxMapWidth || testY < 0 || testY >= k_MaxMapHeight)
				{
					// Ray has his the wall
					bHasHitWall = true;

					// Set distance to current depth
					fDistanceToWall = fDepth;
				}
				else
				{
					// Ray is within map bounds, check if hitting wall
					if (map[testY * k_MaxMapWidth + testX] == '#')
					{
						bHasHitWall = true;

						std::vector<std::pair<float, float>> p; //distance 

						//check 4 corners of block
						for (int x = 0; x < 2; x++)
						{
							for (int y = 0; y < 2; y++)
							{
								//get perfect corners
								float cornerX = (float)testX + x - fPlayerX;
								float cornerY = (float)testY + y - fPlayerX;

								//get perfect corner distance to player
								float cornerDistance = sqrt(cornerX * cornerX + cornerY * cornerY);

								//angle between ray and corner
								float dot = (fEyeX * cornerX / cornerDistance) + (fEyeY * cornerY / cornerDistance);

								//add to vector of pairs
								p.push_back(std::make_pair(cornerDistance, dot));
							}
						}

						// Sort pairs to get closest to player
						//go through each pair and return the smaller distance 
						std::sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {return left.first < right.first; });
						
						//radians
						float fBound = 0.01;

						//look for the SMALLEST angle between the player ray and corner ray
						if (std::acos(p.at(0).second) < fBound)
						{
							bHasHitBoundry = true;
						}

						if (std::acos(p.at(1).second) < fBound)
						{
							bHasHitBoundry = true;
						}

						/*if (std::acos(p.at(2).second) < fBound)
						{
							bHasHitBoundry = true;
						}*/
					}
				}

				// Calculate ceiling and floor distance
				int ceiling = (float)(k_MaxScreenHeight / 2.0) - k_MaxScreenHeight / ((float)fDistanceToWall);
				int floor = k_MaxScreenHeight - ceiling;

				//shaders
				short nShade = ' ';

				//assign shader type based on distance
				if (fDistanceToWall <= fDepth / 4.0f)
				{
					nShade = 0x2588; //close up wall
				}
				else if (fDistanceToWall < fDepth / 3.0f)
				{
					nShade = 0x2593;
				}
				else if (fDistanceToWall < fDepth / 2.0f)
				{
					nShade = 0x2592;
				}
				else if (fDistanceToWall < fDepth)
				{
					nShade = 0x2591;
				}
				else
				{
					nShade = ' '; //far away wall
				}

				if (bHasHitBoundry)
				{
					nShade = ' '; //Don't show it
				}

				for (int y = 0; y < k_MaxScreenHeight; y++)
				{
					if (y < ceiling)
					{
						screen[y * k_MaxScreenWidth + x] = ' ';
					}
					else if (y > ceiling && y <= floor)
					{
						screen[y * k_MaxScreenWidth + x] = nShade;
					}
					else
					{

						 // Shade floor
						float b = 1.0f - (((float)y - k_MaxScreenHeight / 2.0f) / ((float)k_MaxScreenHeight / 2.0f));

						if (b < 0.25)
						{
							nShade = '#';
						}
						else if (b < 0.5)
						{
							nShade = 'x';
						}
						else if (b < 0.75)
						{
							nShade = '.';
						}
						else if (b < 0.90)
						{
							nShade = '-';
						}
						
						/*else
						{
							nShade = ' ';
						}*/

						screen[y * k_MaxScreenWidth + x] = nShade;
					}
				}
			}
		}

		// Stat display
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, R=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerRotation, 1.0f / fElapsedTime);
		// Show map
		for (int x = 0; x < k_MaxMapWidth; x++)
		{
			for (int y = 0; y < k_MaxMapWidth; y++)
			{
				// offset by 1
				screen[(y + 1) * k_MaxScreenWidth + x] = map[y * k_MaxMapWidth + x];
			}

		}

		//draw player location on map
		screen[((int)fPlayerY + 1) * k_MaxScreenWidth + (int)fPlayerX] = 'P';

		// Set last char of array to 'eol' for limit
		screen[k_MaxScreenWidth * k_MaxScreenHeight - 1] = '\0';

		// Write text to top write corner (0, 0)
		WriteConsoleOutputCharacter(hConsole, screen, k_MaxScreenHeight * k_MaxScreenWidth, { 0, 0 }, &dwBytesWritten);
	}

	return 0;

}

