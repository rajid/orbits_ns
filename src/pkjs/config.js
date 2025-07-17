module.exports = [
  {
    "type": "heading",
    "defaultValue": "Orbits_wf Configuration"
  },
  {
      "type": "section",
      "items": [
	{
            "type": "heading",
            "defaultValue": "Settings"
        },
	{
	    "type": "toggle",
            "messageKey": "displayMonth",
            "label": "Display Month",
            "defaultValue": false
        },
	{
	    "type": "toggle",
            "messageKey": "displayDate",
            "label": "Display Date",
            "defaultValue": false
        },
	{
	    "type": "toggle",
            "messageKey": "displayHour",
            "label": "Display Hour",
            "defaultValue": false
        },
	{
	    "type": "toggle",
            "messageKey": "displayMinute",
            "label": "Display Minute",
            "defaultValue": false
        },
	{
	    "type": "toggle",
            "messageKey": "displayBluetooth",
            "label": "Display Bluetooth",
            "defaultValue": false
        },
	{
	    "type": "toggle",
            "messageKey": "updates",
            "label": "Subscribe for updates",
            "defaultValue": false
        },
	{
            "type": "input",
            "messageKey": "ago",
            "label": "Pod changed in the past? (mins",
            "defaultValue": "0"
	},
	{
            "type": "input",
            "messageKey": "timer",
            "label": "Run a timer? (minutes)",
            "defaultValue": "0"
	},
	{
	    "type": "toggle",
            "messageKey": "changeit",
            "label": "Pod change now?",
            "defaultValue": false
        },
	{
            "type": "submit",
            "id": "submit",
            "defaultValue": "save"
        }
      ]
  }
]

      
