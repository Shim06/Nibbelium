#include "main.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                                                    | SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED);
    SDL_Window* window = SDL_CreateWindow("Nibbelium", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }

    // Get current directory
    std::filesystem::path current_directory_path = std::filesystem::current_path();
    std::string current_directory = current_directory_path.string();

    // Load window icon
    SDL_Surface* icon_surface = SDL_LoadBMP((current_directory + "\\icon.bmp").c_str());
    if (icon_surface != NULL) {
        SDL_SetWindowIcon(window, icon_surface);
        SDL_FreeSurface(icon_surface);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    SetupImGuiStyle();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);


    // Load Font
    io.Fonts->AddFontFromFileTTF((current_directory + "\\Fonts\\Tamsyn10x20.ttf").c_str(), 18.0f);

    // Initialize variables
    bool show_settings_window = false;
    bool fullscreen_on = false;
    bool start_games_fullscreen;
    bool display_wait;
    bool logic_quirk;
    bool wrapping_quirk;
    int volume;
    int IPS_value;
    ImVec4 pixel_on_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 pixel_off_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    // initialize chip8 emulator
    chip8 emulator;

    // Load settings json
    std::ifstream config_file(current_directory + "\\config.json");

    if (config_file.is_open())
    {

        // Get values from json file
        nlohmann::json config;
        config_file >> config;
        config_file.close();

        volume = config["volume"];
        display_wait = config["display_wait"];
        IPS_value = config["IPS"];
        logic_quirk = config["logic"];
        wrapping_quirk = config["wrapping"];
        start_games_fullscreen = config["start_games_fullscreen"];

        // Normalized 
        pixel_on_color.x = config["pixel_on_color_R"] / 255.0f;   // Red
        pixel_on_color.y = config["pixel_on_color_G"] / 255.0f;   // Green
        pixel_on_color.z = config["pixel_on_color_B"] / 255.0f;   // Blue

        pixel_off_color.x = config["pixel_off_color_R"] / 255.0f; // Red
        pixel_off_color.y = config["pixel_off_color_G"] / 255.0f; // Green
        pixel_off_color.z = config["pixel_off_color_B"] / 255.0f; // Blue
    }
    else
    {
        // If the json doesn't exist, create a new one with default values
        nlohmann::json config;

        config["pixel_on_color_R"] = 255;
        config["pixel_on_color_G"] = 204;
        config["pixel_on_color_B"] = 1;
        config["pixel_off_color_R"] = 153;
        config["pixel_off_color_G"] = 102;
        config["pixel_off_color_B"] = 1;
        config["volume"] = 100;
        config["display_wait"] = false;
        config["IPS"] = 700;
        config["logic"] = true;
        config["wrapping"] = false;
        config["start_games_fullscreen"] = false;

        std::ofstream newConfigFile("config.json");
        newConfigFile << std::setw(4) << config;
        newConfigFile.close();

        volume = config["volume"];
        display_wait = config["display_wait"];
        IPS_value = config["IPS"];
        logic_quirk = config["logic"];
        wrapping_quirk = config["wrapping"];
        start_games_fullscreen = config["start_games_fullscreen"];

        // Normalized
        pixel_on_color.x = config["pixel_on_color_R"] / 255.0f;   // Red
        pixel_on_color.y = config["pixel_on_color_G"] / 255.0f;   // Green
        pixel_on_color.z = config["pixel_on_color_B"] / 255.0f;   // Blue

        pixel_off_color.x = config["pixel_off_color_R"] / 255.0f; // Red
        pixel_off_color.y = config["pixel_off_color_G"] / 255.0f; // Green
        pixel_off_color.z = config["pixel_off_color_B"] / 255.0f; // Blue
    }

    // Get list of games from game folder
    std::string folderPath = current_directory + "\\Games";
    std::vector<std::string> ch8_roms = GetCh8FileNames(folderPath);

    // Sort the games alphabetically
    std::sort(ch8_roms.begin(), ch8_roms.end());


    // Main loop
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    bool done = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_o:
                        if (SDL_GetModState() & KMOD_CTRL)
                        {
                            std::string rom_path = LoadROM();
                            if (rom_path != "")
                            {
                                emulator.emulate(rom_path, current_directory);
                            }
                        }
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();


        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowPos({ 0, 0 });

        ImGui::Begin("##NoLabel", nullptr, ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar()) {
            // File menu
            if (ImGui::BeginMenu("File"))
            {

                // Load rom button
                if (ImGui::MenuItem("Load Chip-8 Rom", "Ctrl+O"))
                {
                    std::string rom_path = LoadROM();
                    if (rom_path != "")
                    {
                        emulator.emulate(rom_path, current_directory);
                    }
                }
                if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_O)))
                {
                    std::string rom_path = LoadROM();
                    if (rom_path != "")
                    {
                        emulator.emulate(rom_path, current_directory);
                    }
                }
                ImGui::Separator();

                // Open game folder button
                if (ImGui::MenuItem("Open Game Folder"))
                {
                    ShellExecute(NULL, "open", "explorer.exe", (current_directory + "\\Games").c_str(), NULL, SW_SHOWNORMAL);
                }
                ImGui::Separator();

                // Exit Button
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    ImGui_ImplSDLRenderer2_Shutdown();
                    ImGui_ImplSDL2_Shutdown();
                    ImGui::DestroyContext();

                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(window);
                    SDL_Quit();
                    return 0;
                }
                ImGui::EndMenu();
            }

            // Options menu
            if (ImGui::BeginMenu("Options"))
            {

                if (fullscreen_on == false)
                {
                    if (ImGui::MenuItem("Enter Fullscreen"))
                    {
                        fullscreen_on = true;
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Exit Fullscreen"))
                    {
                        fullscreen_on = false;
                        SDL_SetWindowFullscreen(window, 0);
                    }
                }


                if (ImGui::MenuItem("Start Games in Fullscreen mode", nullptr, &start_games_fullscreen))
                {
                    // Save to json
                    std::ifstream config_file(current_directory + "\\config.json");
                    nlohmann::json config;
                    config_file >> config;

                    config["start_games_fullscreen"] = start_games_fullscreen;

                    std::ofstream fileStream(current_directory + "\\config.json");
                    fileStream << std::setw(4) << config << std::endl;
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Settings"))
                {
                    show_settings_window = true;
                }
                ImGui::EndMenu();
            }
            // End the menu bar
            ImGui::EndMenuBar();
        }

        // Settings 
        if (show_settings_window)
        {
            ImGui::SetNextWindowSize(ImGui::GetWindowSize());
            ImGui::SetNextWindowPos(ImVec2(0, 0));

            if (ImGui::Begin("Settings", &show_settings_window, ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
            {
                // Settings window content goes here
                ImGui::Text("Emulator Settings");

                ImGui::Text("Pixel On Color");
                ImGui::ColorEdit4("##Color1", (float*)&pixel_on_color, ImGuiColorEditFlags_NoAlpha);

                // Color picker for Color 2
                ImGui::Text("Pixel Off Color");
                ImGui::ColorEdit4("##Color2", (float*)&pixel_off_color, ImGuiColorEditFlags_NoAlpha);

                ImGui::Text("Volume");
                ImGui::SliderInt("##Volume", &volume, 0, 100);

                ImGui::Text("Instructions per second (IPS): ");
                // Instructions per second
                if (ImGui::InputInt("##IPS", &IPS_value, 1, 0, ImGuiInputTextFlags_CharsDecimal |
                           ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsNoBlank))
                {
                    if (IPS_value < 60) IPS_value = 60;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("The rate at which the emulator executes instructions within a second. Recommended IPS is 600-800");
                }

                ImGui::Text("Emulator Quirks");

                // Toggle Display wait
                ImGui::Checkbox("Display Wait", &display_wait);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Display wait is a way to better emulate the behaviour of the original CHIP-8 interpreter on the COSMAC VIP.");
                }

                ImGui::Checkbox("Logic Quirk", &logic_quirk);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("If false, The VF register is not touched when performing logical operations. This is used for modern Chip-8 games");
                }

                ImGui::Checkbox("Wrapping Quirk", &wrapping_quirk);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Sprites that go beyond the boundaries of the display area reappear on the opposite side.");
                }

                if (ImGui::Button("Close")) {
                    // Save settings to json
                    std::ifstream config_file(current_directory + "\\config.json");
                    nlohmann::json config;
                    config_file >> config;

                    config["pixel_on_color_R"] = (int)(pixel_on_color.x * 255);
                    config["pixel_on_color_G"] = (int)(pixel_on_color.y * 255);
                    config["pixel_on_color_B"] = (int)(pixel_on_color.z * 255);
                    config["pixel_off_color_R"] = (int)(pixel_off_color.x * 255);
                    config["pixel_off_color_G"] = (int)(pixel_off_color.y * 255);
                    config["pixel_off_color_B"] = (int)(pixel_off_color.z * 255);
                    config["volume"] = volume;
                    config["display_wait"] = display_wait;
                    config["logic"] = logic_quirk;
                    config["wrapping"] = wrapping_quirk;
                    config["IPS"] = IPS_value;

                    std::ofstream fileStream(current_directory + "\\config.json");
                    fileStream << std::setw(4) << config << std::endl;

                    show_settings_window = false;
                }
            }
            ImGui::End();
        }

        // Games list
        ImGui::Text("List of Games:");

        // Using ListBox to display the game names
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO", "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO", "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_current_idx = 0; // Here we store our selection data as an index.
        if (ImGui::BeginListBox("##NoLabel", ImGui::GetContentRegionAvail()))
        {
            for (int n = 0, size = ch8_roms.size(); n < size; n++)
            {
                const bool is_selected = (item_current_idx == n);
                const char* game_name = ch8_roms[n].c_str();
                if (ImGui::Selectable(game_name, is_selected))
                    item_current_idx = n;

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && item_current_idx == n) {

                    std::string game_path = current_directory + "\\Games\\" + ch8_roms[item_current_idx];
                    emulator.emulate(game_path, current_directory);
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

std::string LoadROM()
{
    OPENFILENAME ofn;
    TCHAR sz_file[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; 
    ofn.lpstrFile = sz_file;
    ofn.nMaxFile = sizeof(sz_file) / sizeof(*sz_file);
    ofn.lpstrFilter = (".ch8\0*.ch8\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        return sz_file;
    }

    return "";
}

std::vector<std::string> GetCh8FileNames(const std::string& folderPath)
{
    std::vector<std::string> file_names;

    // Construct the search path (e.g., "C:\\Path\\To\\Folder\\*.ch8")
    std::string search_path = folderPath + "\\*.ch8";

    WIN32_FIND_DATA find_file_data;
    HANDLE h_find = FindFirstFile(search_path.c_str(), &find_file_data);

    if (h_find != INVALID_HANDLE_VALUE) {
        do {
            // Add the file name to the vector
            file_names.push_back(find_file_data.cFileName);
        } while (FindNextFile(h_find, &find_file_data) != 0);

        // Close the handle
        FindClose(h_find);
    }

    return file_names;
}

void SetupImGuiStyle()
{
    // Moonlight style by deathsu/madam-herta
    // https://github.com/Madam-Herta/Moonlight/
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 1.0f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(20.0f, 3.400000095367432f);
    style.FrameRounding = 11.89999961853027f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(4.300000190734863f, 5.5f);
    style.ItemInnerSpacing = ImVec2(7.099999904632568f, 1.799999952316284f);
    style.CellPadding = ImVec2(12.10000038146973f, 9.199999809265137f);
    style.IndentSpacing = 0.0f;
    style.ColumnsMinSpacing = 4.900000095367432f;
    style.ScrollbarSize = 11.60000038146973f;
    style.ScrollbarRounding = 15.89999961853027f;
    style.GrabMinSize = 3.700000047683716f;
    style.GrabRounding = 20.0f;
    style.TabRounding = 0.0f;
    style.TabBorderSize = 0.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09250493347644806f, 0.100297249853611f, 0.1158798336982727f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1120669096708298f, 0.1262156516313553f, 0.1545064449310303f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.971993625164032f, 1.0f, 0.4980392456054688f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.7953379154205322f, 0.4980392456054688f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1821731775999069f, 0.1897992044687271f, 0.1974248886108398f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1545050293207169f, 0.1545048952102661f, 0.1545064449310303f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1414651423692703f, 0.1629818230867386f, 0.2060086131095886f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1072951927781105f, 0.107295036315918f, 0.1072961091995239f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1293079704046249f, 0.1479243338108063f, 0.1931330561637878f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1459212601184845f, 0.1459220051765442f, 0.1459227204322815f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.999999463558197f, 1.0f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1249424293637276f, 0.2735691666603088f, 0.5708154439926147f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8841201663017273f, 0.7941429018974304f, 0.5615870356559753f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9570815563201904f, 0.9570719599723816f, 0.9570761322975159f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9356134533882141f, 0.9356129765510559f, 0.9356223344802856f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.266094446182251f, 0.2890366911888123f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}
