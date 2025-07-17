//
// Customizing Clay's colors to be green instead of red
//
module.exports = function(minified) {
    var clayConfig = this;
    var _ = minified._;
    var $ = minified.$;
    var HTML = minified.HTML;

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {

            // General function for toggling color
            function toggle_item(e) {
                // only set the background if toggled
                var toggled = e.get();
                e.$element.select('.marker').set('$backgroundColor',  toggled ? 'green' : null);
                e.$element.select('.slide').set('$backgroundColor', toggled ? 'darkgreen' : null);
            };

            // Our callback for changes made to an item
            function toggle_it() {
                // only set the background if toggled
                toggle_item(this);
            };
            
            // Setup change callbacks
            clayConfig.getItemByMessageKey('displayMonth').on('change', toggle_it);
            clayConfig.getItemByMessageKey('displayDate').on('change', toggle_it);
            clayConfig.getItemByMessageKey('displayHour').on('change', toggle_it);
            clayConfig.getItemByMessageKey('displayMinute').on('change', toggle_it);
            clayConfig.getItemByMessageKey('displayBluetooth').on('change', toggle_it);
            clayConfig.getItemByMessageKey('updates').on('change', toggle_it);
            clayConfig.getItemByMessageKey('changeit').on('change', toggle_it);

            // Set initial state colors
            toggle_item(clayConfig.getItemByMessageKey('displayMonth'));
            toggle_item(clayConfig.getItemByMessageKey('displayDate'));
            toggle_item(clayConfig.getItemByMessageKey('displayHour'));
            toggle_item(clayConfig.getItemByMessageKey('displayMinute'));
            toggle_item(clayConfig.getItemByMessageKey('displayBluetooth'));
            toggle_item(clayConfig.getItemByMessageKey('updates'));
            toggle_item(clayConfig.getItemByMessageKey('changeit'));

            // Set background of "Send" button
            clayConfig.getItemById('submit').$element.select('button').set({$backgroundColor: 'green'});
        })
};

