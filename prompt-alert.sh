# Set RESMAN_ALERT to a red `!` when the system is reserved
update_resman_alert() {
    local lock_file="/etc/res_lock"

    # Check if the file exists and is not empty
    if [ -s "$lock_file" ]; then
        # Check the status of the lock
        current_time=$(date +%s)
        is_locked=$(python3 -c "import sys, json; data=json.load(sys.stdin); print(data.get('duration', 0) == -1 or data.get('start_time', 0) + data.get('duration', 0) > $current_time)" < "$lock_file")

        if [ "$is_locked" = "True" ]; then
            RESMAN_ALERT=$'\e[31m!\e[0m'
        else
            # Remove the old lock here, for faster future checks
            truncate -s 0 /etc/res_lock
            RESMAN_ALERT=""
        fi
    else
        RESMAN_ALERT=""
    fi
}

PS1='${RESMAN_ALERT}\u@\h:\w> '

# Recompute RESMAN_ALERT every time the prompt is refreshed
PROMPT_COMMAND="update_resman_alert;${PROMPT_COMMAND:+$PROMPT_COMMAND}"
