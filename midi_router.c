/**
 * SimpleMidiRouter - A simple MIDI routing utility
 * Using Win32 API and Windows Multimedia API (winmm)
 */

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdbool.h>
#include <commctrl.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

// Constants
#define MAX_MIDI_DEVICES 32
#define MAX_MIDI_CHANNELS 16
#define WM_MIDI_DATA (WM_USER + 1)
#define IDC_INPUT_LIST 101
#define IDC_OUTPUT_LIST 102
#define IDC_CONNECT_BTN 103
#define IDC_DISCONNECT_BTN 104
#define IDC_CONNECTIONS_LIST 105
#define IDC_CHANNELS_BTN 106
#define IDC_STATUS_BAR 107
#define IDC_SELECT_ALL_BTN 301
#define IDC_SELECT_NONE_BTN 302

// Global variables
HWND g_hWnd;
HWND g_hInputList;
HWND g_hOutputList;
HWND g_hConnectionsList;
HWND g_hStatusBar;

// MIDI device information
UINT g_numInputDevices = 0;
UINT g_numOutputDevices = 0;
HMIDIIN g_midiInHandles[MAX_MIDI_DEVICES] = {0};
HMIDIOUT g_midiOutHandles[MAX_MIDI_DEVICES] = {0};
bool g_routingMatrix[MAX_MIDI_DEVICES][MAX_MIDI_DEVICES] = {0};
bool g_channelMatrix[MAX_MIDI_DEVICES][MAX_MIDI_CHANNELS] = {0};

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitializeMidiDevices(void);

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

void UpdateConnectionsList(void);

void CreateConnection(int inDeviceIdx, int outDeviceIdx);

void RemoveConnection(int inDeviceIdx, int outDeviceIdx);

BOOL CALLBACK ChannelDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize Common Controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    // Register window class
    const char CLASS_NAME[] = "SimpleMidiRouterClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Create window
    g_hWnd = CreateWindowEx(
        0, // Optional window styles
        CLASS_NAME, // Window class
        "Simple MIDI Router", // Window text
        WS_OVERLAPPEDWINDOW, // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, // Position
        800, 600, // Size
        NULL, // Parent window
        NULL, // Menu
        hInstance, // Instance handle
        NULL // Additional data
    );

    if (g_hWnd == NULL) {
        MessageBox(NULL, "Window Creation Failed", "Error", MB_ICONERROR);
        return 0;
    }

    // Create controls
    g_hInputList = CreateWindowEx(
        WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        20, 40, 200, 200,
        g_hWnd, (HMENU) IDC_INPUT_LIST, hInstance, NULL
    );

    g_hOutputList = CreateWindowEx(
        WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        240, 40, 200, 200,
        g_hWnd, (HMENU) IDC_OUTPUT_LIST, hInstance, NULL
    );

    g_hConnectionsList = CreateWindowEx(
        WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        460, 40, 300, 200,
        g_hWnd, (HMENU) IDC_CONNECTIONS_LIST, hInstance, NULL
    );

    CreateWindowEx(
        0, "BUTTON", "Connect",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 250, 100, 30,
        g_hWnd, (HMENU) IDC_CONNECT_BTN, hInstance, NULL
    );

    CreateWindowEx(
        0, "BUTTON", "Disconnect",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        130, 250, 100, 30,
        g_hWnd, (HMENU) IDC_DISCONNECT_BTN, hInstance, NULL
    );

    CreateWindowEx(
        0, "BUTTON", "Channel Filter",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        240, 250, 100, 30,
        g_hWnd, (HMENU) IDC_CHANNELS_BTN, hInstance, NULL
    );

    // Create status bar
    g_hStatusBar = CreateWindowEx(
        0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        g_hWnd, (HMENU) IDC_STATUS_BAR, hInstance, NULL
    );

    // Add labels
    CreateWindowEx(
        0, "STATIC", "MIDI Inputs:",
        WS_CHILD | WS_VISIBLE,
        20, 20, 100, 20,
        g_hWnd, NULL, hInstance, NULL
    );

    CreateWindowEx(
        0, "STATIC", "MIDI Outputs:",
        WS_CHILD | WS_VISIBLE,
        240, 20, 100, 20,
        g_hWnd, NULL, hInstance, NULL
    );

    CreateWindowEx(
        0, "STATIC", "Active Connections:",
        WS_CHILD | WS_VISIBLE,
        460, 20, 150, 20,
        g_hWnd, NULL, hInstance, NULL
    );

    // Initialize MIDI
    InitializeMidiDevices();

    // Initialize channel matrix - enable all channels by default
    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        for (int j = 0; j < MAX_MIDI_CHANNELS; j++) {
            g_channelMatrix[i][j] = true;
        }
    }

    // Show window
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Message loop
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup MIDI resources
    for (UINT i = 0; i < g_numInputDevices; i++) {
        if (g_midiInHandles[i]) {
            midiInStop(g_midiInHandles[i]);
            midiInClose(g_midiInHandles[i]);
        }
    }

    for (UINT i = 0; i < g_numOutputDevices; i++) {
        if (g_midiOutHandles[i]) {
            midiOutClose(g_midiOutHandles[i]);
        }
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case IDC_CONNECT_BTN: {
                    int inIdx = SendMessage(g_hInputList, LB_GETCURSEL, 0, 0);
                    int outIdx = SendMessage(g_hOutputList, LB_GETCURSEL, 0, 0);

                    if (inIdx >= 0 && outIdx >= 0) {
                        CreateConnection(inIdx, outIdx);
                        UpdateConnectionsList();

                        char statusMsg[256];
                        sprintf(statusMsg, "Connected input %d to output %d", inIdx, outIdx);
                        SendMessage(g_hStatusBar, SB_SETTEXT, 0, (LPARAM) statusMsg);
                    }
                }
                break;

                case IDC_DISCONNECT_BTN: {
                    int connIdx = SendMessage(g_hConnectionsList, LB_GETCURSEL, 0, 0);
                    if (connIdx >= 0) {
                        char connText[256];
                        SendMessage(g_hConnectionsList, LB_GETTEXT, connIdx, (LPARAM) connText);

                        // Parse connection string "Input X -> Output Y"
                        int inIdx = -1, outIdx = -1;
                        sscanf(connText, "Input %d -> Output %d", &inIdx, &outIdx);

                        if (inIdx >= 0 && outIdx >= 0) {
                            RemoveConnection(inIdx, outIdx);
                            UpdateConnectionsList();

                            char statusMsg[256];
                            sprintf(statusMsg, "Disconnected input %d from output %d", inIdx, outIdx);
                            SendMessage(g_hStatusBar, SB_SETTEXT, 0, (LPARAM) statusMsg);
                        }
                    }
                }
                break;

                case IDC_CHANNELS_BTN: {
                    int outIdx = SendMessage(g_hOutputList, LB_GETCURSEL, 0, 0);
                    if (outIdx >= 0) {
                        // Create a modal dialog for channel filtering
                        DialogBoxParam(GetModuleHandle(NULL), "CHANNEL_DIALOG", hwnd, ChannelDialogProc, outIdx);
                    }
                }
                break;
            }
        }
        break;

        case WM_MIDI_DATA: {
            HMIDIIN hMidiIn = (HMIDIIN) wParam;
            DWORD midiMessage = (DWORD) lParam;

            // Find the input device index
            int inIdx = -1;
            for (UINT i = 0; i < g_numInputDevices; i++) {
                if (g_midiInHandles[i] == hMidiIn) {
                    inIdx = i;
                    break;
                }
            }

            if (inIdx >= 0) {
                // Get the message status byte and channel (if applicable)
                BYTE status = midiMessage & 0xFF;
                BYTE channel = status & 0x0F;
                BYTE msgType = status & 0xF0;

                // Route to all connected outputs with channel filtering
                for (UINT outIdx = 0; outIdx < g_numOutputDevices; outIdx++) {
                    if (g_routingMatrix[inIdx][outIdx] && g_midiOutHandles[outIdx]) {
                        bool shouldRoute = true;

                        // Apply channel filtering for channel messages
                        if (msgType >= 0x80 && msgType <= 0xE0) {
                            shouldRoute = g_channelMatrix[outIdx][channel];
                        }

                        if (shouldRoute) {
                            midiOutShortMsg(g_midiOutHandles[outIdx], midiMessage);

                            // Update status bar (optional, but useful for debugging)
                            char statusMsg[256];
                            sprintf(statusMsg, "Routed: 0x%08X from In %d to Out %d", midiMessage, inIdx, outIdx);
                            SendMessage(g_hStatusBar, SB_SETTEXT, 0, (LPARAM) statusMsg);
                        }
                    }
                }
            }
        }
        break;

        case WM_SIZE: {
            // Resize the status bar when the window is resized
            SendMessage(g_hStatusBar, WM_SIZE, 0, 0);
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void InitializeMidiDevices(void) {
    // Get number of MIDI input devices
    g_numInputDevices = midiInGetNumDevs();
    if (g_numInputDevices > MAX_MIDI_DEVICES)
        g_numInputDevices = MAX_MIDI_DEVICES;

    // Populate input list
    for (UINT i = 0; i < g_numInputDevices; i++) {
        MIDIINCAPS midiInCaps;
        if (midiInGetDevCaps(i, &midiInCaps, sizeof(MIDIINCAPS)) == MMSYSERR_NOERROR) {
            char deviceName[256];
            sprintf(deviceName, "%d: %s", i, midiInCaps.szPname);
            SendMessage(g_hInputList, LB_ADDSTRING, 0, (LPARAM) deviceName);
        }
    }

    // Get number of MIDI output devices
    g_numOutputDevices = midiOutGetNumDevs();
    if (g_numOutputDevices > MAX_MIDI_DEVICES)
        g_numOutputDevices = MAX_MIDI_DEVICES;

    // Populate output list
    for (UINT i = 0; i < g_numOutputDevices; i++) {
        MIDIOUTCAPS midiOutCaps;
        if (midiOutGetDevCaps(i, &midiOutCaps, sizeof(MIDIOUTCAPS)) == MMSYSERR_NOERROR) {
            char deviceName[256];
            sprintf(deviceName, "%d: %s", i, midiOutCaps.szPname);
            SendMessage(g_hOutputList, LB_ADDSTRING, 0, (LPARAM) deviceName);
        }
    }

    // Set status
    char statusMsg[256];
    sprintf(statusMsg, "Found %d MIDI inputs and %d MIDI outputs", g_numInputDevices, g_numOutputDevices);
    SendMessage(g_hStatusBar, SB_SETTEXT, 0, (LPARAM) statusMsg);
}

// Callback function for MIDI input
void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
    switch (wMsg) {
        case MIM_DATA:
            // Forward the message to the main window for routing
            PostMessage(g_hWnd, WM_MIDI_DATA, (WPARAM) hMidiIn, (LPARAM) dwParam1);
            break;

        case MIM_LONGDATA:
            // Handle System Exclusive messages (not implemented in this example)
            break;

        case MIM_ERROR:
        case MIM_LONGERROR:
            // Handle errors
            PostMessage(g_hWnd, WM_MIDI_DATA, (WPARAM) hMidiIn, (LPARAM) 0);
            break;
    }
}

void UpdateConnectionsList(void) {
    // Clear the list
    SendMessage(g_hConnectionsList, LB_RESETCONTENT, 0, 0);

    // Add all active connections
    for (UINT i = 0; i < g_numInputDevices; i++) {
        for (UINT j = 0; j < g_numOutputDevices; j++) {
            if (g_routingMatrix[i][j]) {
                char connectionText[256];
                sprintf(connectionText, "Input %d -> Output %d", i, j);
                SendMessage(g_hConnectionsList, LB_ADDSTRING, 0, (LPARAM) connectionText);
            }
        }
    }
}

void CreateConnection(int inDeviceIdx, int outDeviceIdx) {
    // Check valid indices
    if (inDeviceIdx < 0 || inDeviceIdx >= (int) g_numInputDevices ||
        outDeviceIdx < 0 || outDeviceIdx >= (int) g_numOutputDevices) {
        return;
    }

    // Update routing matrix
    g_routingMatrix[inDeviceIdx][outDeviceIdx] = true;

    // Open MIDI input device if not already open
    if (!g_midiInHandles[inDeviceIdx]) {
        MMRESULT result = midiInOpen(&g_midiInHandles[inDeviceIdx], inDeviceIdx,
                                     (DWORD_PTR) MidiInProc, 0, CALLBACK_FUNCTION);

        if (result == MMSYSERR_NOERROR) {
            midiInStart(g_midiInHandles[inDeviceIdx]);
        } else {
            MessageBox(g_hWnd, "Failed to open MIDI input device", "Error", MB_ICONERROR);
        }
    }

    // Open MIDI output device if not already open
    if (!g_midiOutHandles[outDeviceIdx]) {
        MMRESULT result = midiOutOpen(&g_midiOutHandles[outDeviceIdx], outDeviceIdx, 0, 0, CALLBACK_NULL);

        if (result != MMSYSERR_NOERROR) {
            MessageBox(g_hWnd, "Failed to open MIDI output device", "Error", MB_ICONERROR);
        }
    }
}

void RemoveConnection(int inDeviceIdx, int outDeviceIdx) {
    // Check valid indices
    if (inDeviceIdx < 0 || inDeviceIdx >= (int) g_numInputDevices ||
        outDeviceIdx < 0 || outDeviceIdx >= (int) g_numOutputDevices) {
        return;
    }

    // Update routing matrix
    g_routingMatrix[inDeviceIdx][outDeviceIdx] = false;

    // Check if input device is used by any other connection
    bool inputStillUsed = false;
    for (UINT j = 0; j < g_numOutputDevices; j++) {
        if (g_routingMatrix[inDeviceIdx][j]) {
            inputStillUsed = true;
            break;
        }
    }

    // Close MIDI input device if no longer used
    if (!inputStillUsed && g_midiInHandles[inDeviceIdx]) {
        midiInStop(g_midiInHandles[inDeviceIdx]);
        midiInClose(g_midiInHandles[inDeviceIdx]);
        g_midiInHandles[inDeviceIdx] = NULL;
    }

    // Check if output device is used by any other connection
    bool outputStillUsed = false;
    for (UINT i = 0; i < g_numInputDevices; i++) {
        if (g_routingMatrix[i][outDeviceIdx]) {
            outputStillUsed = true;
            break;
        }
    }

    // Close MIDI output device if no longer used
    if (!outputStillUsed && g_midiOutHandles[outDeviceIdx]) {
        midiOutClose(g_midiOutHandles[outDeviceIdx]);
        g_midiOutHandles[outDeviceIdx] = NULL;
    }
}

// Dialog proc for channel filtering
BOOL CALLBACK ChannelDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int s_outputDeviceIdx = -1;
    static HWND s_channelCheckboxes[MAX_MIDI_CHANNELS] = {0};

    switch (uMsg) {
        case WM_INITDIALOG: {
            s_outputDeviceIdx = (int) lParam;

            // Set the dialog title
            char title[256];
            sprintf(title, "Channel Filter for Output %d", s_outputDeviceIdx);
            SetWindowText(hwnd, title);

            // Create checkboxes for each channel - arrange in 2 columns for better layout
            for (int i = 0; i < MAX_MIDI_CHANNELS; i++) {
                char channelText[32];
                sprintf(channelText, "Channel %d", i + 1);

                // Position in 2 columns for better layout
                int x = (i < 8) ? 20 : 160;
                int y = 20 + (i % 8) * 30;

                s_channelCheckboxes[i] = CreateWindowEx(
                    0, "BUTTON", channelText,
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    x, y, 120, 20,
                    hwnd, (HMENU) (200 + i), GetModuleHandle(NULL), NULL
                );

                // Set the initial state based on the channel matrix
                SendMessage(s_channelCheckboxes[i], BM_SETCHECK,
                            g_channelMatrix[s_outputDeviceIdx][i] ? BST_CHECKED : BST_UNCHECKED, 0);
            }

            // Add Select All and Select None buttons
            CreateWindowEx(
                0, "BUTTON", "Select All",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 280, 120, 30,
                hwnd, (HMENU) IDC_SELECT_ALL_BTN, GetModuleHandle(NULL), NULL
            );

            CreateWindowEx(
                0, "BUTTON", "Select None",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                160, 280, 120, 30,
                hwnd, (HMENU) IDC_SELECT_NONE_BTN, GetModuleHandle(NULL), NULL
            );

            // Create OK and Cancel buttons
            CreateWindowEx(
                0, "BUTTON", "OK",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                80, 340, 80, 30,
                hwnd, (HMENU) IDOK, GetModuleHandle(NULL), NULL
            );

            CreateWindowEx(
                0, "BUTTON", "Cancel",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                170, 340, 80, 30,
                hwnd, (HMENU) IDCANCEL, GetModuleHandle(NULL), NULL
            );
        }
            return TRUE;

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case IDC_SELECT_ALL_BTN:
                    // Check all channel checkboxes
                    for (int i = 0; i < MAX_MIDI_CHANNELS; i++) {
                        SendMessage(s_channelCheckboxes[i], BM_SETCHECK, BST_CHECKED, 0);
                    }
                    return TRUE;

                case IDC_SELECT_NONE_BTN:
                    // Uncheck all channel checkboxes
                    for (int i = 0; i < MAX_MIDI_CHANNELS; i++) {
                        SendMessage(s_channelCheckboxes[i], BM_SETCHECK, BST_UNCHECKED, 0);
                    }
                    return TRUE;

                case IDOK:
                    // Save channel settings
                    for (int i = 0; i < MAX_MIDI_CHANNELS; i++) {
                        g_channelMatrix[s_outputDeviceIdx][i] =
                                (SendMessage(s_channelCheckboxes[i], BM_GETCHECK, 0, 0) == BST_CHECKED);
                    }
                    EndDialog(hwnd, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
