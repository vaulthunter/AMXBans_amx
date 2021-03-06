/*

	AMXBans, managing bans for Half-Life modifications
	Copyright (C) 2003, 2004  Ronald Renes / Jeroen de Rover
	
	Copyright (C) 2009, 2010  Thomas Kurz

*/

#if defined _check_player_included
    #endinput
#endif
#define _check_player_included

public prebanned_check(id) {
	if(is_user_bot(id) || id==0)
		return PLUGIN_HANDLED
	
	if(!get_cvarptr_num(pcvar_show_prebanned))
		return PLUGIN_HANDLED
	
	if(get_user_flags(id) & ADMIN_IMMUNITY)
		return PLUGIN_HANDLED
	
	new player_steamid[35], player_ip[22];
	get_user_authid(id, player_steamid, 34)
	get_user_ip(id, player_ip, 21, 1)
	
	mysql_query(g_SqlX, "SELECT COUNT(*) FROM `%s%s` WHERE ( (player_id='%s' AND ban_type='S') OR (player_ip='%s' AND ban_type='SI') ) AND expired=1",g_dbPrefix, tbl_bans, player_steamid, player_ip)
	
	prebanned_check_(id);
	return PLUGIN_HANDLED
}
public prebanned_check_(id)
{
	if(!mysql_num_rows(g_SqlX))
		return PLUGIN_HANDLED;
		
	mysql_nextrow(g_SqlX);
		
	new ban_count=mysql_getfield(g_SqlX, 1)
	
	if(ban_count < get_cvarptr_num(pcvar_show_prebanned_num))
		return PLUGIN_HANDLED
		
	new name[32], player_steamid[35]
	get_user_authid(id, player_steamid, 34)
	get_user_name(id, name, 31)
	
	for(new i=1;i<=g_iMaxPlayers;i++) {
		if(i==id || is_user_bot(i) || is_user_hltv(i) || !is_user_connected(i)) continue
		if(get_user_flags(i) & ADMIN_CHAT) {
			client_print(i, print_chat, _T("[AMXBans] <%s> %s has been banned %i times before."), name, player_steamid, ban_count)
		}
	}
	log_amx(_T("[AMXBans] <%s> %s has been banned %i times before."), name, player_steamid, ban_count)
	
	return PLUGIN_HANDLED
}

/*************************************************************************/

public check_player(id) {
	new player_steamid[32], player_ip[20]
	get_user_authid(id, player_steamid, 31)
	get_user_ip(id, player_ip, 19, 1)
	
	mysql_query(g_SqlX, "SELECT bid,ban_created,ban_length,UNIX_TIMESTAMP(NOW()),ban_reason,admin_nick,admin_id,admin_ip,player_nick,player_id,player_ip,server_name,server_ip,ban_type \
		FROM `%s%s` WHERE ( (player_id='%s' AND ban_type='S') OR (player_ip='%s' AND ban_type='SI') ) AND expired=0",g_dbPrefix, tbl_bans, player_steamid, player_ip);
		
	check_player_(id);
	
	return PLUGIN_HANDLED
}

public check_player_(id)
{
	if(!mysql_num_rows(g_SqlX)) {
		check_flagged(id)
		return PLUGIN_HANDLED
	}
	
	new ban_reason[128], admin_nick[100],admin_steamid[50],admin_ip[30],ban_type[4]
	new player_nick[50],player_steamid[50],player_ip[30],server_name[100],server_ip[30]
	
	mysql_nextrow(g_SqlX);
	
	new bid = mysql_getfield(g_SqlX, 1)
	new ban_created = mysql_getfield(g_SqlX, 2)
	new ban_length_int = mysql_getfield(g_SqlX, 3)*60 //min to sec
	new current_time_int = mysql_getfield(g_SqlX, 4)
	mysql_getfield(g_SqlX, 5, ban_reason, 127)
	mysql_getfield(g_SqlX, 6, admin_nick, 99)
	mysql_getfield(g_SqlX, 7, admin_steamid, 49)
	mysql_getfield(g_SqlX, 8, admin_ip, 29)
	mysql_getfield(g_SqlX, 9, player_nick, 49)
	mysql_getfield(g_SqlX, 10, player_steamid, 49)
	mysql_getfield(g_SqlX, 11, player_ip, 29)
	mysql_getfield(g_SqlX, 12, server_name, 99)
	mysql_getfield(g_SqlX, 13, server_ip, 29)
	mysql_getfield(g_SqlX, 14, ban_type, 3)

	if ( get_cvarptr_num(pcvar_debug) >= 1 )
		log_amx("[AMXBans] Player Check on Connect:^nbid: %d ^nwhen: %d ^nlenght: %d ^nreason: %s ^nadmin: %s ^nadminsteamID: %s ^nPlayername %s ^nserver: %s ^nserverip: %s ^nbantype: %s",\
		bid,ban_created,ban_length_int,ban_reason,admin_nick,admin_steamid,player_nick,server_name,server_ip,ban_type)

	// A ban was found for the connecting player!! Lets see how long it is or if it has expired
	if ((ban_length_int == 0) || (ban_created ==0) || ((ban_created+ban_length_int) > current_time_int)) {
		new complain_url[256]
		get_cvarptr_string(pcvar_complainurl ,complain_url,255)
		
		client_cmd(id, "echo [AMXBans] ===============================================")
		
		new show_activity = get_cvar_num("amx_show_activity")
		
		if(get_user_flags(id)&get_admin_mole_access_flag() || id == 0)
		show_activity = 1
		
		switch(show_activity)
		{
			case 1:
			{
				client_cmd(id, _T("echo [AMXBans] You are banned from this Server!"))
			}
			case 2:
			{
				client_cmd(id, _T("echo [AMXBans] You have been banned from this Server by Admin %s."), admin_nick)
			}
			case 3:
			{
				if (is_user_admin(id))
					client_cmd(id, _T("echo [AMXBans] You have been banned from this Server by Admin %s."), admin_nick)
				else
					client_cmd(id, _T("echo [AMXBans] You are banned from this Server!"))
			}
			case 4:
			{
				if (is_user_admin(id))
					client_cmd(id, _T("echo [AMXBans] You have been banned from this Server by Admin %s."), admin_nick)
			}
			case 5:
			{
				if (is_user_admin(id))
					client_cmd(id, _T("echo [AMXBans] You are banned from this Server!"))
			}
		}
		
		if (ban_length_int==0) {
			client_cmd(id, _T("echo [AMXBans] You are permanently banned."))
		} else {
			new cTimeLength[128]
			new iSecondsLeft = (ban_created + ban_length_int - current_time_int)
			get_time_length(id, iSecondsLeft, timeunit_seconds, cTimeLength, 127)
			client_cmd(id, _T("echo [AMXBans] There are %s left of your ban."), cTimeLength)
		}
		
		replace_all(complain_url,charsmax(complain_url),"http://","")
		
		client_cmd(id, _T("echo [AMXBans] Banned Nickname: %s"), player_nick)
		client_cmd(id, _T("echo [AMXBans] Reason: '%s'"), ban_reason)
		client_cmd(id, _T("echo [AMXBans] You can complain about your ban @ %s"), complain_url)
		client_cmd(id, _T("echo [AMXBans] Your SteamID: '%s'"), player_steamid)
		client_cmd(id, _T("echo [AMXBans] Your IP: '%s'"), player_ip)
		client_cmd(id, "echo [AMXBans] ===============================================")

		if ( get_cvarptr_num(pcvar_debug) >= 1 )
			log_amx("[AMXBans] BID:<%d> Player:<%s> <%s> connected and got kicked, because of an active ban", bid, player_nick, player_steamid)

		new id_str[3]
		num_to_str(id,id_str,3)

		if ( get_cvarptr_num(pcvar_debug) >= 1 )
			log_amx("[AMXBans] Delayed Kick-TASK ID1: <%d>  ID2: <%s>", id, id_str)
		
		add_kick_to_db(bid)
		
		id+=200
		set_task(3.5,"delayed_kick",id)

		return PLUGIN_HANDLED
	} else {
		// The ban has expired
		client_cmd(id, _T("echo [AMXBans] You have been banned at least one time before."))

		mysql_query(g_SqlX, "UPDATE `%s%s` SET expired=1 WHERE bid=%d", g_dbPrefix, tbl_bans, bid)
		
		if ( get_cvarptr_num(pcvar_debug) >= 1 )
			log_amx("[AMXBans] PRUNE BAN: %s",pquery)
			
		check_flagged(id)
	}
	return PLUGIN_HANDLED
}

/*************************************************************************/
public add_kick_to_db(bid)
{
	mysql_query(g_SqlX, "UPDATE `%s%s` SET `ban_kicks`=`ban_kicks`+1 WHERE `bid`=%d",g_dbPrefix, tbl_bans, bid)
	return PLUGIN_HANDLED
}
