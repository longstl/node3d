-- Create Control
listboxServer = CUIListBox()

-- Register Control
IDD_SERVER_LIST:regControl("IDC_LISTBOX_SERVER", listboxServer)

-- Register Event
IDD_SERVER_LIST:regControlEvent("IDC_BTN_OK","IDD_SERVER_LIST_OnBtnOK",1)
IDD_SERVER_LIST:regControlEvent("IDC_LISTBOX_SERVER","IDD_SERVER_LIST_OnBtnOK",1793)

listboxServer:selectByText(L"k")

--- Server List
for line in io.lines("server.ini") do
	_, _, name, ip, port = string.find(line, "([%a.%d]+)[%s]+([%a.%d]+)[%s]+([%d]+)")
	listboxServer:addItem(L(name))
end

-- Event Func
function IDD_SERVER_LIST_OnBtnOK()
	-- g_Audio:PlayMusic("Data/Music/castle.mp3",true,255)
	local serverIndex = listboxServer:getSelectedIndex()
	-- no server
	if serverIndex < 0 then
		MessageBox(L"You did not select a server!")
		return
	end
	-- Connect Server
	local index = 0;
	for line in io.lines("server.ini") do
		_, _, name, ip, port = string.find(line, "([%a.%d]+)[%s]+([%a.%d]+)[%s]+([%d]+)")
		if index == serverIndex then
			if index == 0 then
				g_World:create(0)
				-- 
				g_PlayerMe:setCellPos(127,129)
				g_PlayerMe:setRoleName(L"Local")
				-- Init Equip
				g_PlayerMe:setClass(1)
				g_PlayerMe:setSkeleton()
				g_PlayerMe:setHelm(1)
				g_PlayerMe:setArmor(1)
				g_PlayerMe:setGlove(1)
				g_PlayerMe:setPant(1)
				g_PlayerMe:setBoot(1)
				--
				g_PlayerMe:setDir(1)
				g_PlayerMe:setActionState(0)
				g_World:addRole(g_PlayerMe)
				OnEnterWorld()
			-- Connecting
			elseif ConnectServer(ip,port) then
				IDD_SERVER_LIST:SetVisible(false)
				IDD_LOGIN:SetVisible(true)
			else
				MessageBox(L"Server Connection Failed!")
			end
		end
		index=index+1;
	end
end