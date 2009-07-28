// $Id: apachesolr.js,v 1.1.2.2.2.5 2009/07/28 13:53:37 robertDouglass Exp $

Drupal.behaviors.apachesolr = function(context) {
  $('.apachesolr-hidden-facet').hide();
  $('<a href="#" class="apachesolr-showhide"></a>').text(Drupal.t('Show more')).click(function() {
    if ($(this).prev().find('.apachesolr-hidden-facet:visible').length == 0) {
      $(this).prev().find('.apachesolr-hidden-facet').show();
      $(this).text(Drupal.t('Show fewer'));
    }
    else {
      $(this).prev().find('.apachesolr-hidden-facet').hide();
      $(this).text(Drupal.t('Show more'));
    }
    return false;
  }).appendTo($('.block-apachesolr_search:has(.apachesolr-hidden-facet), .block-apachesolr:has(.apachesolr-hidden-facet)'));
  
  if (Drupal.settings.apachesolr_facetstyle == 'checkboxes') {
    // Find all facet links and give them a checkbox
    $('.apachesolr-facet', context).each(Drupal.apachesolr.addCheckbox);
    // Find all unclick links and turn them into checkboxes
    $('.apachesolr-unclick', context).each(Drupal.apachesolr.makeCheckbox);
  }
}

Drupal.apachesolr = {}

Drupal.apachesolr.addCheckbox = function() {
  // Put href in context scope to be visible in the anonymous function.
  var href = $(this).attr('href'); 
  $(this).before($('<input type="checkbox" />')
    .attr('class', 'facet-checkbox')
    .click(function(){
      window.location.href = href;
    })
  );
}

Drupal.apachesolr.makeCheckbox = function() {
  // Create a checked checkbox.
  var checkbox = $('<input type="checkbox" />')
    .attr('class', 'facet-checkbox')
    .attr('checked', true);
  // Put href in context scope to be visible in the anonymous function.
  var href = $(this).attr('href');
  checkbox.click(function(){
    window.location.href = href;
  });
  // Add the checkbox, hide the link.
  $(this).before(checkbox).hide();
}
