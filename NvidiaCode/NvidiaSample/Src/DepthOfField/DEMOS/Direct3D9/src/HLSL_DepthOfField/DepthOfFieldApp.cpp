#define STRICT
#include "nvafx.h"
#include "DepthOfFieldApp.h"

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
    // Set the callback functions. These functions allow the sample framework to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then the sample framework won't be able to 
    // recreate your device resources.
    DXUTSetCallbackDeviceCreated( OnCreateDevice );
    DXUTSetCallbackDeviceReset( OnResetDevice );
    DXUTSetCallbackDeviceLost( OnLostDevice );
    DXUTSetCallbackDeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackFrameRender( OnFrameRender );
    DXUTSetCallbackFrameMove( OnFrameMove );

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    InitApp();

    // Initialize the sample framework and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true, true ); // Parse the command line, handle the default hotkeys, and show msgboxes
    DXUTCreateWindow( L"Depth of Field demo (DirectX 9)" );
    DXUTCreateDevice( D3DADAPTER_DEFAULT, true, 640, 576, IsDeviceAcceptable, ModifyDeviceSettings );

    // Pass control to the sample framework for handling the message pump and 
    // dispatching render calls. The sample framework will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_pDepthOfFieldApp = new DepthOfField;

    // Initialize dialogs
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_HUD.SetCallback( OnGUIEvent );
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, 34, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, 58, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, 82, 125, 22, VK_F2 );
    g_HUD.AddButton( IDC_TOGGLEWIREFRAME, L"Toggle Wireframe", 35, 106, 125, 22 );

    g_HUD.AddSlider( IDC_FSTOP, 35, 130, 125, 22, 6, 220, g_pDepthOfFieldApp->mFStop * 10);
    g_HUD.AddStatic( IDC_FSTOPTEXT, L"Camera f-stop", 35, 154, 125, 22 );
    g_HUD.AddSlider( IDC_FOCALLENGTH, 35, 188, 125, 22, DepthOfField::kMinFocalLength, DepthOfField::kMaxFocalLength, 20);
    g_HUD.AddStatic( IDC_FOCALLENGTHTEXT, L"Camera focal length", 35, 212, 125, 22 );
    g_HUD.AddSlider( IDC_FOCUSDISTANCE, 35, 236, 125, 22, DepthOfField::kMinFocusDistance, DepthOfField::kMaxFocusDistance, 2536);
    g_HUD.AddStatic( IDC_FOCUSDISTANCETEXT, L"Camera focus distance", 35, 260, 125, 22 );

    g_HUD.AddRadioButton( IDC_RADIO_COLORS, 1, L"Show Colors", 35, 284, 125, 22 );
    g_HUD.GetRadioButton( IDC_RADIO_COLORS )->SetChecked(true);
    g_HUD.AddRadioButton( IDC_RADIO_DEPTH, 1, L"Show Depth", 35, 308, 125, 22 );
    g_HUD.AddRadioButton( IDC_RADIO_BLUR, 1, L"Show Blurriness", 35, 332, 125, 22 );
}


//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning false.
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
                                  D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    BOOL bCapsAcceptable = TRUE;

    // Perform checks to see if these display caps are acceptable.
    bCapsAcceptable = (g_pDepthOfFieldApp->ConfirmDevice(pCaps, 0, AdapterFormat, BackBufferFormat) == S_OK);

    if (bCapsAcceptable)
        return true;
    else
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// the sample framework will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnCreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnCreateDevice( pd3dDevice ) );

    // Initialize the font
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         L"Arial", &g_pFont ) );

    // Set up our view matrix. A view matrix can be defined given an eye point and
    // a point to lookat. Here, we set the eye five units back along the z-axis and 
	// up three units and look at the origin.
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3(0.0f, 1.0f, -25.0f);
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	g_Camera.SetViewParams( &vFromPt, &vLookatPt);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, 
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnResetDevice() );
    V_RETURN( g_SettingsDlg.OnResetDevice() );

    if( g_pFont )
        V_RETURN( g_pFont->OnResetDevice() );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pTextSprite ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 0.1f, 1000.0f );

    g_HUD.SetLocation( 0, 0 );
    g_HUD.SetSize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

	int iY = 15-24;
    for(int i = 0; i < IDC_LAST; i++) {
        g_HUD.GetControl( i )->SetLocation( pBackBufferSurfaceDesc->Width - 135, iY += 24 );
    }

    pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    g_pDepthOfFieldApp->RestoreDeviceObjects(pd3dDevice);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    // TODO: update world
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, the sample framework will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then
    // render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    HRESULT hr;

    // Clear the viewport
    pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET,
                        0x000000ff, 0.0f, 0L);

    // Set the world matrix
    pd3dDevice->SetTransform(D3DTS_WORLD, g_Camera.GetWorldMatrix());

    // Set the projection matrix
    pd3dDevice->SetTransform(D3DTS_PROJECTION, g_Camera.GetProjMatrix());

	// Set the view matrix
	pd3dDevice->SetTransform(D3DTS_VIEW, g_Camera.GetViewMatrix());

	// Begin the scene
    if (SUCCEEDED(pd3dDevice->BeginScene()))
    {
        // render world
        g_pDepthOfFieldApp->Render(pd3dDevice, *g_Camera.GetViewMatrix());

        // Render stats and help text  
        RenderText();

		V( g_HUD.OnRender( fElapsedTime ) );

        // End the scene.
        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    // The helper object simply helps keep track of text position, and color
    // and then it calls pFont->DrawText( m_pSprite, strMsg, -1, &rc, DT_NOCLIP, m_clr );
    // If NULL is passed in as the sprite object, then it will work however the 
    // pFont->DrawText() will not be batched together.  Batching calls will improves performance.
    CDXUTTextHelper txtHelper( g_pFont, g_pTextSprite, 15 );

    // Output statistics
    txtHelper.Begin();
    txtHelper.SetInsertionPos( 5, 15 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    if(g_bShowUI) {
        txtHelper.DrawTextLine( DXUTGetFrameStats() );
        txtHelper.DrawTextLine( DXUTGetDeviceStats() );
    }

    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    
    // Draw help
    if(g_bShowHelp) {
        const D3DSURFACE_DESC* pd3dsdBackBuffer = DXUTGetBackBufferSurfaceDesc();
        txtHelper.SetInsertionPos( 2, pd3dsdBackBuffer->Height-15*6 );
        txtHelper.SetForegroundColor( D3DXCOLOR(1.0f, 0.75f, 0.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Controls:" );

        txtHelper.SetInsertionPos( 20, pd3dsdBackBuffer->Height-15*5 );
        txtHelper.DrawTextLine( L"Navigate: Arrow keys (or w,a,s,d), PgUp/PgDn\n"
                                L"Rotate camera: Mouse button\n"
                                L"Zoom camera: Mouse wheel scroll\n" );

        txtHelper.SetInsertionPos( 300, pd3dsdBackBuffer->Height-15*5 );
        txtHelper.DrawTextLine( L"Home - Reset To Defaults\n"
                                L"Hide help: F1\n" 
                                L"Quit: ESC\n" 
                                L"Toggle UI: H" );
    } else {
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Press F1 for help" );
    }

    txtHelper.End();
}


//--------------------------------------------------------------------------------------
// Before handling window messages, the sample framework passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then the sample framework will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Always allow dialog resource manager calls to handle global messages
    // so GUI state is updated correctly
    g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );

    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// As a convenience, the sample framework inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	if( bKeyDown )
	{
		switch( nChar )
		{
		case VK_F1:
			{
				g_bShowHelp = !g_bShowHelp;
				break;
			}
        case VK_HOME:
            {
                g_Camera.Reset();
                g_pDepthOfFieldApp->mFStop = 1.0f;       
                g_pDepthOfFieldApp->mFocalLength = 20.0f;
                g_pDepthOfFieldApp->mFocusDistance = 2536.0f;
                g_pDepthOfFieldApp->UpdateCameraParameters();
                g_pDepthOfFieldApp->GenerateCircleOfConfusionTexture();
            }
            break;
		case 'H':
		case 'h':
			{
				if( g_bShowUI = !g_bShowUI )
					for( int i = 0; i < IDC_LAST; i++ )
						g_HUD.GetControl(i)->SetVisible( true );
				else
					for( int i = 0; i < IDC_LAST; i++ )
						g_HUD.GetControl(i)->SetVisible( false );
				break;
			}
		}
	}
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN: 
            DXUTToggleFullScreen(); 
            break;
        case IDC_TOGGLEREF:        
            DXUTToggleREF(); 
            break;
        case IDC_CHANGEDEVICE:     
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() );
            break;
        case IDC_TOGGLEWIREFRAME:  
            g_pDepthOfFieldApp->mbWireFrame = !g_pDepthOfFieldApp->mbWireFrame; 
            break;
        case IDC_FSTOP:            
            g_pDepthOfFieldApp->mFStop = 0.1f * g_HUD.GetSlider( nControlID )->GetValue();
            g_pDepthOfFieldApp->UpdateCameraParameters();
            g_pDepthOfFieldApp->GenerateCircleOfConfusionTexture();
            break;
        case IDC_FOCALLENGTH:
            g_pDepthOfFieldApp->mFocalLength = g_HUD.GetSlider( nControlID )->GetValue();
            g_pDepthOfFieldApp->UpdateCameraParameters();
            if(!g_pDepthOfFieldApp->mbUsesVolumes) {
                g_pDepthOfFieldApp->GenerateCircleOfConfusionTexture();
            }
            break;
        case IDC_FOCUSDISTANCE:
            g_pDepthOfFieldApp->mFocusDistance = g_HUD.GetSlider( nControlID )->GetValue();
            g_pDepthOfFieldApp->UpdateCameraParameters();
            break;
        case IDC_RADIO_COLORS:
            g_pDepthOfFieldApp->meDisplayOption = DepthOfField::SHOW_COLORS;
            break;
        case IDC_RADIO_DEPTH:
            g_pDepthOfFieldApp->meDisplayOption = DepthOfField::SHOW_DEPTH;
            break;
        case IDC_RADIO_BLUR:
            g_pDepthOfFieldApp->meDisplayOption = DepthOfField::SHOW_BLURRINESS;
            break;
    }
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnLostDevice();
    g_SettingsDlg.OnLostDevice();

    g_pDepthOfFieldApp->InvalidateDeviceObjects(DXUTGetD3DDevice());

    if( g_pFont )
        g_pFont->OnLostDevice();

	SAFE_RELEASE(g_pTextSprite);
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnDestroyDevice();
    g_SettingsDlg.OnDestroyDevice();

    SAFE_RELEASE(g_pFont);
}