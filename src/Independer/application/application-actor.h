/*
 * ####################################
 *  Who is near? Section
 * ####################################
 */

struct S_I_Application_Device_Item
{
  String deviceId;
  String deviceMsg;
  String receivedRssi;
  String attempt;
};

void application_actor_who_is_in_my_area()
{

  boolean sync_was_on_flag = multi_actor_get_state();

  if (sync_was_on_flag)
  {
    multi_actor_stop();
  }

  int c_max_ping_retries = 3;                                                             // Maximial attempts to receive
  int c_max_ping_delta = C_INDEPENDER_RES_BETWEEN_DELAY_ACTOR;                            // Waiting 1ms between receiving
  int c_max_ping_max_receive_attempts = (C_INDEPENDER_SCAN_MS + 2000) / c_max_ping_delta; // Waiting approx C_INDEPENDER_SCAN_MS seconds for next packet

  S_I_Application_Device_Item collected_db[30];
  int collected_counter = 0;

  int l_attempt = 0;
  while (l_attempt < c_max_ping_retries)
  {
    l_attempt++;

    lora_send_msg_short_message(state_my_id, "*", C_INDEPENDER_SHORT_MESSAGE_CHAR_ALL, state_lora_gain);
    delay(C_INDEPENDER_SEND_DELAY_REPEAT);
    lora_send_msg_short_message(state_my_id, "*", C_INDEPENDER_SHORT_MESSAGE_CHAR_ALL, state_lora_gain);

    gui_display_prg_static("Umgebungs-Scan", l_attempt, 0, c_max_ping_retries);

    int l_cur_receive_attempt = 0;
    while (l_cur_receive_attempt < c_max_ping_max_receive_attempts)
    {
      l_cur_receive_attempt++;

      int packetSize = LoRa.parsePacket();

      if (packetSize)
      {

        String i_res = "";
        for (int i = 0; i < packetSize; i++)
        {
          i_res += (char)LoRa.read();
        }

        ParserAnsTuple parser_ans = lora_stateful_parse(i_res, state_my_id);

        if (parser_ans.message != "")
        {
          struct S_I_Application_Device_Item ret = {
              parser_ans.from,
              parser_ans.message,
              String(LoRa.packetRssi(), DEC),
              String(l_attempt)};
          collected_db[collected_counter] = ret;
          collected_counter++;
        }
      }

      delay(c_max_ping_delta);
    }
  }

  if (collected_counter > 0)
  {
    String gui_items[collected_counter + 1]; // Add + 1 (go back item)

    for (int i = 0; i < collected_counter; i++)
    {
      String r = collected_db[i].attempt + ": (" + collected_db[i].deviceId + ") '" + collected_db[i].deviceMsg + "' RS=" + collected_db[i].receivedRssi;
      // Serial.println("Scan Item '" + r + "'");
      gui_items[i] = r;
    }

    gui_items[collected_counter] = "[zurück]"; // Add go back item

    boolean hasComp = false;
    while (!hasComp)
    {
      int selected = gui_selection("Scan Ausgabe", gui_items, collected_counter - 1 + 1, false); // Add + 1 (go back item)
      if (selected == collected_counter)
        hasComp = true;
      else
        gui_msg_long_text("Scan Detail", gui_items[selected]);
    }
  }

  if (sync_was_on_flag)
  {
    multi_actor_start();
  }
}

/*
 * ####################################
 *  Is Available Section
 * ####################################
 */

/**
 * @return boolean return if is available
 */
boolean application_actor_is_available(String target_id, boolean flagHideAns, String testSendMsg = C_INDEPENDER_SHORT_MESSAGE_CHAR_SINGLE, int maxRetries = 5, String responseMsgCheck = "", boolean checkResponse = false)
{

  boolean sync_was_on_flag = multi_actor_get_state();

  if (sync_was_on_flag)
  {
    multi_actor_stop();
  }

  int c_max_ping_retries = maxRetries;                                                    // Maximial attempts to receive pong message
  int c_max_ping_delta = C_INDEPENDER_RES_BETWEEN_DELAY_ACTOR;                            // Waiting 1ms between receiving
  int c_max_ping_max_receive_attempts = (C_INDEPENDER_SEND_DELAY * 4) / c_max_ping_delta; // Waiting approx 2 seconds for next packet

  String receivedMsg;
  boolean receivedSuccess = false;

  int l_attempt = 0;
  while (l_attempt < c_max_ping_retries and !receivedSuccess)
  {
    l_attempt++;

    gui_display_prg_static("Erreichbar-Check Versuch", l_attempt, 0, c_max_ping_retries);

    lora_send_msg_short_message(state_my_id, target_id, testSendMsg, state_lora_gain);
    delay(C_INDEPENDER_SEND_DELAY_REPEAT);
    lora_send_msg_short_message(state_my_id, target_id, testSendMsg, state_lora_gain);

    int l_cur_receive_attempt = 0;
    while (l_cur_receive_attempt < c_max_ping_max_receive_attempts and !receivedSuccess)
    {
      l_cur_receive_attempt++;

      boolean appendDebug = !checkResponse;

      struct S_APP_PONG pong_ans = application_independer_pong(target_id, appendDebug);

      if (pong_ans.receivedSomething)
      {
        l_cur_receive_attempt = 0;
      }

      if (pong_ans.receivingCompleted and (!checkResponse or pong_ans.message == responseMsgCheck))
      {
        receivedMsg = pong_ans.message;
        receivedSuccess = true;
      }
      else
      {
        delay(c_max_ping_delta);
      }
    }
  }

  if (receivedSuccess and !flagHideAns)
  {
    gui_msg_animated("Antwort", receivedMsg, C_GUI_DELAY_MSG_MIDDLE_I);
  }
  else if (!flagHideAns)
  {
    gui_msg_animated("Fehler", "keine Antwort\nerhalten", C_GUI_DELAY_MSG_MIDDLE_I);
  }

  if (sync_was_on_flag)
  {
    multi_actor_start();
  }

  return receivedSuccess;
}

/*
 * ####################################
 *  Send Message to Gateway Section
 * ####################################
 */

boolean application_actor_send_msg_to_gateway(String receiverId, String userMsg)
{

  boolean sync_was_on_flag = multi_actor_get_state();

  if (sync_was_on_flag)
  {
    multi_actor_stop();
  }

  boolean isAvailable = application_actor_is_available(state_gateway_id, true);

  if (!isAvailable)
  {
    gui_msg_animated("Fehler", "Gateway ist\nnicht erreichbar", C_GUI_DELAY_MSG_MIDDLE_I);
    if (sync_was_on_flag)
    {
      multi_actor_start();
    }
    return false;
  }

  int c_max_ping_retries = 3;                                                             // Maximial attempts to receive pong message
  int c_max_ping_delta = C_INDEPENDER_RES_BETWEEN_DELAY_ACTOR;                            // Waiting 1ms between receiving
  int c_max_ping_max_receive_attempts = (C_INDEPENDER_SEND_DELAY * 4) / c_max_ping_delta; // Waiting approx 2 seconds for next packet

  boolean sendSuccess = false;

  int l_attempt = 0;
  while (l_attempt < c_max_ping_retries and !sendSuccess)
  {
    l_attempt++;

    gui_display_prg_static("Sende Versuch", l_attempt, 0, c_max_ping_retries);

    delay(C_INDEPENDER_SEND_DELAY);

    lora_send_msg(state_my_id, state_gateway_id, "M;" + receiverId + ";" + userMsg, state_lora_gain);

    int l_cur_receive_attempt = 0;
    while (l_cur_receive_attempt < c_max_ping_max_receive_attempts and !sendSuccess)
    {
      l_cur_receive_attempt++;

      struct S_APP_PONG pong_ans = application_independer_pong(state_gateway_id, false);

      if (pong_ans.receivedSomething)
      {
        l_cur_receive_attempt = 0;
      }

      if (pong_ans.receivingCompleted and pong_ans.message == "A;ok")
      {
        sendSuccess = true;
      }
      else
      {
        delay(c_max_ping_delta);
      }
    }
  }

  if (sync_was_on_flag)
  {
    multi_actor_start();
  }

  if (sendSuccess)
  {
    gui_msg_animated("Info", "Nachricht wurde\ngesendet", C_GUI_DELAY_MSG_MIDDLE_I);
  }
  else
  {
    gui_msg_animated("Fehler", "Nachricht konnte\nnicht gesendet werden", C_GUI_DELAY_MSG_MIDDLE_I);
  }

  return sendSuccess;
}

/*
 * ####################################
 *  Query Messages from Gateway Section
 * ####################################
 */

void application_actor_query_msgs_from_gateway()
{

  boolean sync_was_on_flag = multi_actor_get_state();

  if (sync_was_on_flag)
  {
    multi_actor_stop();
  }

  int c_max_ping_retries = 3;                                                             // Maximial attempts to receive pong message
  int c_max_ping_delta = C_INDEPENDER_RES_BETWEEN_DELAY_ACTOR;                            // Waiting 1ms between receiving
  int c_max_ping_max_receive_attempts = (C_INDEPENDER_SEND_DELAY * 4) / c_max_ping_delta; // Waiting approx 2 seconds for next packet

  boolean resSuccess = false;

  int l_attempt = 0;
  while (l_attempt < c_max_ping_retries and !resSuccess)
  {
    l_attempt++;

    gui_display_prg_static("Versuch", l_attempt, 0, c_max_ping_retries);

    delay(C_GUI_DELAY_STATIC_SHORT);

    lora_send_msg_single_unsafe(state_my_id, state_gateway_id, "Q;msg", state_lora_gain);
    delay(C_INDEPENDER_SEND_DELAY_REPEAT);
    lora_send_msg_single_unsafe(state_my_id, state_gateway_id, "Q;msg", state_lora_gain);

    int l_cur_receive_attempt = 0;
    while (l_cur_receive_attempt < c_max_ping_max_receive_attempts and !resSuccess)
    {
      l_cur_receive_attempt++;

      struct S_APP_PONG pong_ans = application_independer_pong(state_gateway_id, false);

      if (pong_ans.receivedSomething)
      {
        l_cur_receive_attempt = 0;
      }

      if (pong_ans.receivingCompleted and pong_ans.message.startsWith("A;"))
      {
        resSuccess = true;

        pong_ans.message = pong_ans.message.substring(2, pong_ans.message.length());

        // Get Num of Messages
        int numMessages = 0;

        String m_buffer_nm = "";
        int currentCountsDelimiter = 0;
        boolean finParser = false;
        for (int i_single_char = 0; i_single_char < pong_ans.message.length() and !finParser; i_single_char++)
        {
          String current = pong_ans.message.substring(i_single_char, i_single_char + 1);
          if (current == ";")
            currentCountsDelimiter++;
          else
          {
            if (currentCountsDelimiter == 0)
              m_buffer_nm += current;
            else if (currentCountsDelimiter == 1)
            {
              pong_ans.message = pong_ans.message.substring(i_single_char, pong_ans.message.length());
              numMessages = m_buffer_nm.toInt();
              finParser = true;
            }
          }
        }

        if (numMessages > 0)
        {

          // Get Messages
          String gui_items[numMessages + 1]; // +1 for go back
          int lastDelimiterIndex = 0;

          for (int i_msg = 0; i_msg < numMessages; i_msg++)
          {

            String r = "";

            String tmp = pong_ans.message.substring(lastDelimiterIndex, pong_ans.message.length());

            // Parse Message Substring

            String m_buffer_author = "";
            String m_buffer_msg = "";
            int currentCountsDelimiter = 0;
            boolean finParser = false;
            for (int i_char_t = 0; i_char_t < tmp.length() and !finParser; i_char_t++)
            {
              String current = tmp.substring(i_char_t, i_char_t + 1);
              if (current == ";")
                currentCountsDelimiter++;
              else
              {
                if (currentCountsDelimiter == 0)
                  m_buffer_author += current;
                else if (currentCountsDelimiter == 1)
                  m_buffer_msg += current;

                if (currentCountsDelimiter == 2 or i_char_t == tmp.length() - 1)
                {

                  // Write message
                  r = "(" + m_buffer_author + ") " + m_buffer_msg;

                  lastDelimiterIndex = lastDelimiterIndex + i_char_t;
                  finParser = true;
                }
              }
            }

            // Serial.println("Message Item '" + r + "'");
            gui_items[i_msg] = r;
          }

          gui_items[numMessages] = "[zurück]"; // Add go back item

          boolean hasMesageShown = false;
          while (!hasMesageShown)
          {
            int selected = gui_selection("Nachrichten", gui_items, numMessages - 1 + 1, false); // Add + 1 (go back item)
            if (selected == numMessages)
              hasMesageShown = true;
            else
              gui_msg_long_text("Nachricht", gui_items[selected]);
          }
        }
        else
          gui_msg_animated("Info", "keine Nachrichten\nvorhanden", C_GUI_DELAY_MSG_MIDDLE_I);
      }
      else
      {
        delay(c_max_ping_delta);
      }
    }
  }

  if (sync_was_on_flag)
  {
    multi_actor_start();
  }

  if (!resSuccess)
  {
    gui_msg_animated("Fehler", "Nachricht konnten\nnicht empfangen werden", C_GUI_DELAY_MSG_MIDDLE_I);
  }
}

/*
 * ####################################
 *  Send Message Actor to Actor Section
 * ####################################
 */

boolean application_actor_send_msg_actor_to_actor(String receiverId, String userMsg)
{

  boolean sync_was_on_flag = multi_actor_get_state();

  if (sync_was_on_flag)
  {
    multi_actor_stop();
  }

  int c_max_ping_retries = 6;                                                             // Maximial attempts to receive pong message
  int c_max_ping_delta = C_INDEPENDER_RES_BETWEEN_DELAY_ACTOR;                            // Waiting 1ms between receiving
  int c_max_ping_max_receive_attempts = (C_INDEPENDER_SEND_DELAY * 4) / c_max_ping_delta; // Waiting approx 2 seconds for next packet

  boolean sendSuccess = false;

  int l_attempt = 0;
  while (l_attempt < c_max_ping_retries and !sendSuccess)
  {
    l_attempt++;

    gui_display_prg_static("Sende Versuch", l_attempt, 0, c_max_ping_retries);

    delay(C_GUI_DELAY_STATIC_SHORT);

    boolean actorAva = application_actor_is_available(receiverId, true, "T", 1, "S", true);

    Serial.println("\nSending Message to Actor " + String(l_attempt) + " of " + String(c_max_ping_retries) + " Actor Ava=" + String(actorAva));

    if (!actorAva)
      delay(C_INDEPENDER_SEND_DELAY);
    else
    {

      delay(C_INDEPENDER_SEND_DELAY);
      lora_send_msg(state_my_id, receiverId, userMsg, state_lora_gain);

      int l_cur_receive_attempt = 0;
      while (l_cur_receive_attempt < c_max_ping_max_receive_attempts and !sendSuccess)
      {
        l_cur_receive_attempt++;

        struct S_APP_PONG pong_ans = application_independer_pong(receiverId, false);

        if (pong_ans.receivedSomething)
        {
          l_cur_receive_attempt = 0;
        }

        if (pong_ans.receivingCompleted and pong_ans.message == "N")
        {
          sendSuccess = true;
        }
        else
        {
          delay(c_max_ping_delta);
        }
      }
    }
  }

  if (sync_was_on_flag)
  {
    multi_actor_start();
  }

  if (sendSuccess)
  {
    gui_msg_animated("Info", "Nachricht wurde\ngesendet", C_GUI_DELAY_MSG_MIDDLE_I);
  }
  else
  {
    gui_msg_animated("Fehler", "Nachricht konnte\nnicht gesendet werden", C_GUI_DELAY_MSG_MIDDLE_I);
  }

  return sendSuccess;
}