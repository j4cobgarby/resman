# Set RESMAN_ALERT to a red `!` when the system is reserved
update_resman_alert() {
    if resman; then
        RESMAN_ALERT=""
    else
        RESMAN_ALERT=$'\e[31m!\e[0m'
    fi
}

PS1='${RESMAN_ALERT}'$PS1

# Recompute RESMAN_ALERT every time the prompt is refreshed
PROMPT_COMMAND="update_resman_alert;${PROMPT_COMMAND:+$PROMPT_COMMAND}"
