function _resman_prompt_command {
    resman || printf "\033[0;31m*****************\n* SERVER IN USE *\n*****************\n\033[0m";
}

PROMPT_COMMAND=_resman_prompt_command
